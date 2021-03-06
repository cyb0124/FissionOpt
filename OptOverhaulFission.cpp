#include <xtensor/xio.hpp>
#include <xtensor/xrandom.hpp>
#include "OverhaulFissionNet.h"

namespace OverhaulFission {
  void Opt::restart() {
    std::shuffle(allowedCoords.begin(), allowedCoords.end(), rng);
    std::copy(settings.limits, settings.limits + Tiles::Air, parent.limits);
    std::copy(settings.sourceLimits, settings.sourceLimits + 3, parent.sourceLimits);
    parent.cellLimits.clear();
    for (auto &fuel : settings.fuels)
      parent.cellLimits.emplace_back(fuel.limit);
    parent.state = xt::broadcast<int>(Tiles::Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    for (auto &[x, y, z] : allowedCoords) {
      int nSym(getNSym(x, y, z));
      allowedTiles.clear();
      for (int tile{}; tile < Tiles::Air; ++tile)
        if (parent.limits[tile] < 0 || parent.limits[tile] >= nSym)
          allowedTiles.emplace_back(tile);
      for (int cell{}; cell < static_cast<int>(settings.cellTypes.size()); ++cell) {
        auto &[fuel, source](settings.cellTypes[cell]);
        if (parent.cellLimits[fuel] >= 0 && parent.cellLimits[fuel] < nSym)
          continue;
        if (source && parent.sourceLimits[source - 1] >= 0 && parent.sourceLimits[source - 1] < nSym)
          continue;
        allowedTiles.emplace_back(Tiles::C0 + cell);
      }
      if (allowedTiles.empty())
        break;
      int newTile(allowedTiles[std::uniform_int_distribution<>(0, static_cast<int>(allowedTiles.size() - 1))(rng)]);
      if (newTile < Tiles::Air) {
        parent.limits[newTile] -= nSym;
      } else {
        auto &[fuel, source](settings.cellTypes[newTile - Tiles::C0]);
        parent.cellLimits[fuel] -= nSym;
        if (source)
          parent.sourceLimits[source - 1] -= nSym;
      }
      setTileWithSym(parent, x, y, z, newTile);
    }
    parent.value.run(parent.state);
    if (settings.controllable)
      parent.valueWithShield.run(parent.state);
  }

  Opt::Opt(Settings &settings)
    :settings(settings),
    nEpisode(), nStage(StageRollout), nIteration(), nConverge(),
    penalty(xt::ones<double>({nConstraints})),
    hasFeasible(xt::zeros<bool>({nConstraints})),
    hasInfeasible(xt::zeros<bool>({nConstraints})),
    bestChanged(true), redrawNagle(), lossHistory(nLossHistory), lossChanged() {
    settings.compute();
    for (int x(settings.symX ? settings.sizeX / 2 : 0); x < settings.sizeX; ++x)
      for (int y(settings.symY ? settings.sizeY / 2 : 0); y < settings.sizeY; ++y)
        for (int z(settings.symZ ? settings.sizeZ / 2 : 0); z < settings.sizeZ; ++z)
          allowedCoords.emplace_back(x, y, z);

    parent.value.initialize(settings, false);
    if (settings.controllable)
      parent.valueWithShield.initialize(settings, true);
    restart();
    net = std::make_unique<Net>(*this);
    net->appendTrajectory(net->extractFeatures(parent));
    parentFitness = currentFitness(parent);
    localBest = xt::all(feasible(parent)) ? parentFitness : 0.0;

    child.value.initialize(settings, false);
    if (settings.controllable)
      child.valueWithShield.initialize(settings, true);

    best.state = xt::broadcast<int>(Tiles::Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    best.value.initialize(settings, false);
    best.value.run(best.state);
  }

  xt::xtensor<bool, 1> Opt::feasible(const Sample &x) {
    return {
      !x.value.totalPositiveNetHeat,
      !settings.controllable || !x.valueWithShield.nActiveCells
    };
  }

  xt::xtensor<double, 1> Opt::infeasibility(const Sample &x) {
    return {
      static_cast<double>(x.value.totalPositiveNetHeat) / settings.minHeat,
      settings.controllable ? x.valueWithShield.nActiveCells : 0.0
    };
  }

  double Opt::rawFitness(const Evaluation &x) {
    switch (settings.goal) {
      default: // GoalOutput
        return x.output / settings.maxOutput;
      case GoalFuelUse:
        return x.nActiveCells;
      case GoalEfficiency:
        return x.efficiency;
      case GoalIrradiation:
        return static_cast<double>(x.irradiatorFlux) / settings.minCriticality;
    }
  }

  double Opt::currentFitness(const Sample &x) {
    if (nStage == StageInfer) {
      return net->infer(x);
    } else if (nStage == StageTrain) {
      return 0.0;
    } else {
      double result(rawFitness(x.value));
      result += std::min(x.value.totalRawFlux, settings.minCriticality) / static_cast<double>(settings.minCriticality);
      result += std::min(x.value.maxCellFlux, settings.minCriticality) / static_cast<double>(settings.minCriticality);
      result -= xt::sum(infeasibility(x) * penalty)();
      return result;
    }
  }

  int Opt::getNSym(int x, int y, int z) {
    int result(1);
    if (settings.symX && x != settings.sizeX - x - 1)
      result *= 2;
    if (settings.symY && y != settings.sizeY - y - 1)
      result *= 2;
    if (settings.symZ && z != settings.sizeZ - z - 1)
      result *= 2;
    return result;
  }

  void Opt::setTileWithSym(Sample &sample, int x, int y, int z, int tile) {
    sample.state(x, y, z) = tile;
    if (settings.symX) {
      sample.state(settings.sizeX - x - 1, y, z) = tile;
      if (settings.symY) {
        sample.state(x, settings.sizeY - y - 1, z) = tile;
        sample.state(settings.sizeX - x - 1, settings.sizeY - y - 1, z) = tile;
        if (settings.symZ) {
          sample.state(x, y, settings.sizeZ - z - 1) = tile;
          sample.state(settings.sizeX - x - 1, y, settings.sizeZ - z - 1) = tile;
          sample.state(x, settings.sizeY - y - 1, settings.sizeZ - z - 1) = tile;
          sample.state(settings.sizeX - x - 1, settings.sizeY - y - 1, settings.sizeZ - z - 1) = tile;
        }
      } else if (settings.symZ) {
        sample.state(x, y, settings.sizeZ - z - 1) = tile;
        sample.state(settings.sizeX - x - 1, y, settings.sizeZ - z - 1) = tile;
      }
    } else if (settings.symY) {
      sample.state(x, settings.sizeY - y - 1, z) = tile;
      if (settings.symZ) {
        sample.state(x, y, settings.sizeZ - z - 1) = tile;
        sample.state(x, settings.sizeY - y - 1, settings.sizeZ - z - 1) = tile;
      }
    } else if (settings.symZ) {
      sample.state(x, y, settings.sizeZ - z - 1) = tile;
    }
  }

  void Opt::mutateAndEvaluate(Sample &sample, int x, int y, int z) {
    int nSym(getNSym(x, y, z));
    int oldTile(sample.state(x, y, z));
    if (oldTile < Tiles::Air) {
      sample.limits[oldTile] += nSym;
    } else if (oldTile >= Tiles::C0) {
      auto &[fuel, source](settings.cellTypes[oldTile - Tiles::C0]);
      sample.cellLimits[fuel] += nSym;
      if (source)
        sample.sourceLimits[source - 1] += nSym;
    }
    allowedTiles.clear();
    allowedTiles.emplace_back(Tiles::Air);
    for (int tile{}; tile < Tiles::Air; ++tile)
      if (sample.limits[tile] < 0 || sample.limits[tile] >= nSym)
        allowedTiles.emplace_back(tile);
    for (int cell{}; cell < static_cast<int>(settings.cellTypes.size()); ++cell) {
      auto &[fuel, source](settings.cellTypes[cell]);
      if (sample.cellLimits[fuel] >= 0 && sample.cellLimits[fuel] < nSym)
        continue;
      if (source && sample.sourceLimits[source - 1] >= 0 && sample.sourceLimits[source - 1] < nSym)
        continue;
      allowedTiles.emplace_back(Tiles::C0 + cell);
    }
    int newTile(allowedTiles[std::uniform_int_distribution<>(0, static_cast<int>(allowedTiles.size() - 1))(rng)]);
    if (newTile < Tiles::Air) {
      sample.limits[newTile] -= nSym;
    } else if (newTile >= Tiles::C0) {
      auto &[fuel, source](settings.cellTypes[newTile - Tiles::C0]);
      sample.cellLimits[fuel] -= nSym;
      if (source)
        sample.sourceLimits[source - 1] -= nSym;
    }
    setTileWithSym(sample, x, y, z, newTile);
    sample.value.run(sample.state);
    if (settings.controllable)
      sample.valueWithShield.run(sample.state);
  }

  void Opt::step() {
    if (nStage == StageTrain) {
      if (!nIteration) {
        nStage = StageInfer;
        restart();
        parentFitness = currentFitness(parent);
        inferenceFailed = true;
      } else {
        for (int i{}; i < nLossHistory - 1; ++i)
          lossHistory[i] = lossHistory[i + 1];
        lossHistory[nLossHistory - 1] = net->train();
        lossChanged = true;
        --nIteration;
        return;
      }
    } else if (nStage == StageInfer) {
      if (nConverge == maxConvergeInfer) {
        nStage = StageRollout;
        ++nEpisode;
        std::cout << "episode " << nEpisode << std::endl;
        if (inferenceFailed)
          restart();
        net->newTrajectory();
        net->appendTrajectory(net->extractFeatures(parent));
        parentFitness = currentFitness(parent);
        localBest = xt::all(feasible(parent)) ? parentFitness : 0.0;
        nConverge = 0;
        nIteration = 0;
      }
    } else if (nConverge == maxConvergeRollout) {
      nStage = StageTrain;
      trajectoryBuffer.clear();
      net->finishTrajectory(localBest);
      nConverge = 0;
      nIteration = (net->getTrajectoryLength() * nEpoch + nMiniBatch - 1) / nMiniBatch;
      return;
    }

    bool bestChangedLocal(!nEpisode && nStage == StageRollout && !nIteration && xt::all(feasible(parent)));
    if (bestChangedLocal)
      best = parent;
    std::uniform_int_distribution<>
      xDist(0, settings.sizeX - 1),
      yDist(0, settings.sizeY - 1),
      zDist(0, settings.sizeZ - 1);
    child.state = parent.state;
    std::copy(parent.limits, parent.limits + Tiles::Air, child.limits);
    std::copy(parent.sourceLimits, parent.sourceLimits + 3, child.sourceLimits);
    child.cellLimits = parent.cellLimits;
    mutateAndEvaluate(child, xDist(rng), yDist(rng), zDist(rng));
    double childFitness(currentFitness(child));
    if (xt::all(feasible(child)) && rawFitness(child.value) > rawFitness(best.value)) {
      bestChangedLocal = true;
      best = child;
    }
    if (childFitness >= parentFitness) {
      if (childFitness > parentFitness) {
        parentFitness = childFitness;
        if (nStage == StageInfer) {
          nConverge = 0;
          inferenceFailed = false;
        }
      }
      if (nStage != StageInfer && !std::uniform_int_distribution<>(0, 9)(rng))
-        trajectoryBuffer.emplace_back(net->extractFeatures(child));
      std::swap(parent, child);
    }

    if (nStage != StageInfer) {
      auto feasible(this->feasible(parent));
      if (xt::all(feasible)) {
        if (parentFitness > localBest) {
          localBest = parentFitness;
          std::cout << "localBest: " << localBest << std::endl;
          nConverge = 0;
          while (!trajectoryBuffer.empty()) {
            net->appendTrajectory(std::move(trajectoryBuffer.back()));
            trajectoryBuffer.pop_back();
          }
        }
      }
      for (int i{}; i < nConstraints; ++i)
        if (feasible(i))
          hasFeasible(i) = true;
        else
          hasInfeasible(i) = true;
      if (!(nIteration % penaltyUpdatePeriod)) {
        std::cout << penalty(0) << std::endl;
        for (int i{}; i < nConstraints; ++i) {
          if (hasFeasible(i) && !hasInfeasible(i))
            penalty(i) *= 0.5;
          else if (!hasFeasible(i) && hasInfeasible(i))
            penalty(i) = std::max(0.001, penalty(i) * 1.5);
          hasFeasible(i) = false;
          hasInfeasible(i) = false;
        }
      }
      parentFitness = currentFitness(parent);
    }

    ++nConverge;
    ++nIteration;
    if (bestChangedLocal) {
      best.value.canonicalize(best.state);
      bestChanged = true;
    }
  }

  void Opt::stepInteractive() {
    int dim(settings.sizeX * settings.sizeY * settings.sizeZ);
    int n(std::min(interactiveMin, (interactiveScale + dim - 1) / dim));
    for (int i{}; i < (nStage == StageTrain ? interactiveNet : nStage == StageInfer ? interactiveNet * nMiniBatch : n); ++i) {
      step();
      ++redrawNagle;
    }
  }

  bool Opt::needsRedrawBest() {
    bool result(bestChanged && redrawNagle >= interactiveMin);
    if (result) {
      bestChanged = false;
      redrawNagle = 0;
    }
    return result;
  }

  bool Opt::needsReplotLoss() {
    bool result(lossChanged);
    if (result)
      lossChanged = false;
    return result;
  }
}
