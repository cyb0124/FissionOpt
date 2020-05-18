#include <xtensor/xview.hpp>
#include "FissionNet.h"

namespace Fission {
  void Opt::restart() {
    std::shuffle(allowedCoords.begin(), allowedCoords.end(), rng);
    std::copy(settings.limit, settings.limit + Air, parent.limit);
    parent.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    for (auto &[x, y, z] : allowedCoords) {
      int nSym(getNSym(x, y, z));
      allowedTiles.clear();
      for (int tile{}; tile < Air; ++tile)
        if (parent.limit[tile] < 0 || parent.limit[tile] >= nSym)
          allowedTiles.emplace_back(tile);
      if (allowedTiles.empty())
        break;
      int newTile(allowedTiles[std::uniform_int_distribution<>(0, static_cast<int>(allowedTiles.size() - 1))(rng)]);
      parent.limit[newTile] -= nSym;
      setTileWithSym(parent, x, y, z, newTile);
    }
    evaluator.run(parent.state, parent.value);
  }

  Opt::Opt(const Settings &settings, bool useNet)
    :settings(settings), evaluator(settings),
    nEpisode(), nStage(), nIteration(), nConverge(),
    maxConverge(std::min(7 * 7 * 7, settings.sizeX * settings.sizeY * settings.sizeZ) * 16),
    infeasibilityPenalty(), bestChanged(true), redrawNagle(), lossHistory(nLossHistory), lossChanged() {
    for (int x(settings.symX ? settings.sizeX / 2 : 0); x < settings.sizeX; ++x)
      for (int y(settings.symY ? settings.sizeY / 2 : 0); y < settings.sizeY; ++y)
        for (int z(settings.symZ ? settings.sizeZ / 2 : 0); z < settings.sizeZ; ++z)
          allowedCoords.emplace_back(x, y, z);

    restart();
    if (useNet) {
      net = std::make_unique<Net>(*this);
      net->appendTrajectory(parent);
    }
    parentFitness = currentFitness(parent);

    std::copy(settings.limit, settings.limit + Air, best.limit);
    best.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    evaluator.run(best.state, best.value);
  }

  bool Opt::feasible(const Evaluation &x) {
    return !settings.ensureHeatNeutral || x.netHeat <= 0.0;
  }

  double Opt::rawFitness(const Evaluation &x) {
    switch (settings.goal) {
      default: // GoalPower
        return x.avgMult;
      case GoalBreeder:
        return x.avgBreed;
      case GoalEfficiency:
        return settings.ensureHeatNeutral ? (x.efficiency - 1) * x.dutyCycle : x.efficiency - 1;
    }
  }

  double Opt::currentFitness(const Sample &x) {
    if (nStage == StageInfer) {
      return net->infer(x);
    } else if (nStage == StageTrain) {
      return 0.0;
    } else if (feasible(x.value)) {
      return rawFitness(x.value);
    } else {
      return rawFitness(x.value) - x.value.netHeat / settings.fuelBaseHeat * infeasibilityPenalty;
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
    if (oldTile != Air)
      sample.limit[oldTile] += nSym;
    allowedTiles.clear();
    allowedTiles.emplace_back(Air);
    for (int tile{}; tile < Air; ++tile)
      if (sample.limit[tile] < 0 || sample.limit[tile] >= nSym)
        allowedTiles.emplace_back(tile);
    int newTile(allowedTiles[std::uniform_int_distribution<>(0, static_cast<int>(allowedTiles.size() - 1))(rng)]);
    if (newTile != Air)
      sample.limit[newTile] -= nSym;
    setTileWithSym(sample, x, y, z, newTile);
    evaluator.run(sample.state, sample.value);
  }

  void Opt::step() {
    if (nStage == StageTrain) {
      if (!nIteration) {
        nStage = StageInfer;
        parentFitness = net->infer(parent);
        std::cout << "from: " << parentFitness << std::endl;
        inferenceFailed = true;
      } else {
        for (int i{}; i < nLossHistory - 1; ++i)
          lossHistory[i] = lossHistory[i + 1];
        lossHistory[nLossHistory - 1] = net->train();
        lossChanged = true;
        --nIteration;
        return;
      }
    }

    if (nConverge == maxConverge) {
      nIteration = 0;
      nConverge = 0;
      if (nStage == StageInfer) {
        nStage = 0;
        ++nEpisode;
        std::cout << "to: " << parentFitness;
        if (inferenceFailed) {
          std::cout << ", retry" << std::endl;
          restart();
        } else {
          std::cout << ", continue" << std::endl;
        }
        net->newTrajectory();
        net->appendTrajectory(parent);
      } else if (feasible(parent.value) || infeasibilityPenalty > 1e8) {
        infeasibilityPenalty = 0.0;
        if (net) {
          nStage = StageTrain;
          double ret(feasible(parent.value) ? rawFitness(parent.value) : 0.0);
          std::cout << "return: " << ret << std::endl;
          net->finishTrajectory(ret);
          nIteration = (net->getTrajectoryLength() * nEpoch + nMiniBatch - 1) / nMiniBatch;
          return;
        } else {
          nStage = 0;
          ++nEpisode;
          restart();
        }
      } else {
        ++nStage;
        if (infeasibilityPenalty)
          infeasibilityPenalty *= 2;
        else
          infeasibilityPenalty = std::uniform_real_distribution<>()(rng);
      }
      parentFitness = currentFitness(parent);
    }

    bool bestChangedLocal(!nEpisode && !nStage && !nIteration && feasible(parent.value));
    if (bestChangedLocal)
      best = parent;
    std::uniform_int_distribution<>
      xDist(0, settings.sizeX - 1),
      yDist(0, settings.sizeY - 1),
      zDist(0, settings.sizeZ - 1);
    int bestChild;
    double bestFitness;
    for (int i{}; i < children.size(); ++i) {
      auto &child(children[i]);
      child.state = parent.state;
      std::copy(parent.limit, parent.limit + Air, child.limit);
      mutateAndEvaluate(child, xDist(rng), yDist(rng), zDist(rng));
      double fitness(currentFitness(child));
      if (!i || fitness > bestFitness) {
        bestChild = i;
        bestFitness = fitness;
      }
      if (feasible(child.value) && rawFitness(child.value) > rawFitness(best.value)) {
        bestChangedLocal = true;
        best = child;
      }
    }
    auto &child(children[bestChild]);
    if (bestFitness >= parentFitness) {
      if (bestFitness > parentFitness) {
        parentFitness = bestFitness;
        nConverge = 0;
        if (nStage == StageInfer)
          inferenceFailed = false;
      }
      std::swap(parent, child);
      if (net && nStage != StageInfer)
        net->appendTrajectory(parent);
    }
    ++nConverge;
    ++nIteration;
    if (bestChangedLocal) {
      for (auto &[x, y, z] : best.value.invalidTiles)
        best.state(x, y, z) = Air;
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
