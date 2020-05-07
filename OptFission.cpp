#include "OptFission.h"

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
    infeasibilityPenalty = 0.0;
    nConverge = 0;
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), evaluator(settings), isFirstIteration(true),
    maxConverge(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
    for (int x(settings.symX ? settings.sizeX / 2 : 0); x < settings.sizeX; ++x)
      for (int y(settings.symY ? settings.sizeY / 2 : 0); y < settings.sizeY; ++y)
        for (int z(settings.symZ ? settings.sizeZ / 2 : 0); z < settings.sizeZ; ++z)
          allowedCoords.emplace_back(x, y, z);
    restart();
    best = parent;
  }

  bool Opt::feasible(const Evaluation &x) {
    return !settings.ensureHeatNeutral || x.netHeat <= 0.0;
  }

  double Opt::rawFitness(const Evaluation &x) {
    return settings.breeder ? x.avgBreed : x.avgMult;
  }

  double Opt::penalizedFitness(const Evaluation &x) {
    double result(rawFitness(x));
    if (!feasible(x))
      result -= x.netHeat / settings.fuelBaseHeat * infeasibilityPenalty;
    return result;
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

  bool Opt::step() {
    if (nConverge == maxConverge) {
      if (feasible(parent.value) || infeasibilityPenalty > 1e8) {
        restart();
      } else {
        if (infeasibilityPenalty)
          infeasibilityPenalty *= 2;
        else
          infeasibilityPenalty = std::uniform_real_distribution<>()(rng);
        nConverge = 0;
      }
    }
    bool bestChanged(isFirstIteration && feasible(best.value));
    std::uniform_int_distribution<>
      xDist(0, settings.sizeX - 1),
      yDist(0, settings.sizeY - 1),
      zDist(0, settings.sizeZ - 1);
    int bestChild{};
    for (int i{}; i < children.size(); ++i) {
      auto &child(children[i]);
      child.state = parent.state;
      std::copy(parent.limit, parent.limit + Air, child.limit);
      mutateAndEvaluate(child, xDist(rng), yDist(rng), zDist(rng));
      if (i && penalizedFitness(child.value) > penalizedFitness(children[bestChild].value))
        bestChild = i;
      if (feasible(child.value) && rawFitness(child.value) > rawFitness(best.value)) {
        best = child;
        bestChanged = true;
      }
    }
    auto &child(children[bestChild]);
    if (penalizedFitness(child.value) + 1e-8 >= penalizedFitness(parent.value)) {
      if (penalizedFitness(child.value) > penalizedFitness(parent.value))
        nConverge = 0;
      std::swap(parent, child);
    }
    ++nConverge;
    isFirstIteration = false;
    if (bestChanged)
      for (auto &[x, y, z] : best.value.invalidTiles)
        best.state(x, y, z) = Air;
    return bestChanged;
  }

  bool Opt::stepBatch(int nBatch) {
    bool bestChanged{};
    for (int i{}; i < nBatch; ++i)
      if (step())
        bestChanged = true;
    return bestChanged;
  }
}
