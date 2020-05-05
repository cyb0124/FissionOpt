#include "OptFission.h"

namespace Fission {
  void Opt::restart() {
    std::shuffle(positions.begin(), positions.end(), rng);
    std::copy(settings.limit, settings.limit + Air, parent.limit);
    parent.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    std::vector<int> allTiles;
    for (auto &[x, y, z] : positions) {
      int nSym(getNSym(x, y, z));
      allTiles.clear();
      for (int tile{}; tile < Air; ++tile)
        if (parent.limit[tile] < 0 || parent.limit[tile] >= nSym)
          allTiles.emplace_back(tile);
      if (allTiles.empty())
        break;
      int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
      parent.limit[newTile] -= nSym;
      setTileWithSym(parent, x, y, z, newTile);
    }

    parent.value = evaluate(settings, parent.state, nullptr);
    localUtopia = parent.value;
    localPareto = parent.value;
    penaltyEnabled = false;
    nConverge = 0;
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), maxCooling(),
    maxConverge(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {

    for (int x(settings.symX ? settings.sizeX / 2 : 0); x < settings.sizeX; ++x)
      for (int y(settings.symY ? settings.sizeY / 2 : 0); y < settings.sizeY; ++y)
        for (int z(settings.symZ ? settings.sizeZ / 2 : 0); z < settings.sizeZ; ++z)
          positions.emplace_back(x, y, z);

    for (int i{}; i < Cell; ++i)
      if (settings.limit[i])
        maxCooling = std::max(maxCooling, settings.coolingRates[i]);

    restart();
    globalPareto = parent;
  }

  void Opt::removeInvalidTiles() {
    std::vector<std::tuple<int, int, int>> invalidTiles;
    evaluate(settings, globalPareto.state, &invalidTiles);
    for (auto [x, y, z] : invalidTiles)
      globalPareto.state(x, y, z) = Air;
  }

  bool Opt::feasible(const Evaluation &x) {
    return !settings.ensureHeatNeutral || x.netHeat <= 0.0;
  }

  double Opt::rawFitness(const Evaluation &x) {
    return settings.breeder ? x.avgBreed : x.avgPower;
  }

  double Opt::penalizedFitness(const Evaluation &x) {
    double result(rawFitness(x));
    if (penaltyEnabled && !feasible(x))
      result -= (rawFitness(localUtopia) - rawFitness(localPareto)) * (x.netHeat / maxCooling);
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
    std::vector<int> allTiles{Air};
    for (int tile{}; tile < Air; ++tile)
      if (sample.limit[tile] < 0 || sample.limit[tile] >= nSym)
        allTiles.emplace_back(tile);
    int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
    if (newTile != Air)
      sample.limit[newTile] -= nSym;
    setTileWithSym(sample, x, y, z, newTile);
    sample.value = evaluate(settings, sample.state, nullptr);
  }

  bool Opt::step() {
    if (nConverge == maxConverge) {
      if (!penaltyEnabled && !feasible(localUtopia)) {
        penaltyEnabled = true;
        nConverge = 0;
      } else {
        restart();
      }
    }
    std::uniform_int_distribution<>
      xDist(0, settings.sizeX - 1),
      yDist(0, settings.sizeY - 1),
      zDist(0, settings.sizeZ - 1);
    int bestChild{};
    std::array<Sample, 4> children;
    for (int i{}; i < children.size(); ++i) {
      auto &child(children[i]);
      child.state = parent.state;
      std::copy(parent.limit, parent.limit + Air, child.limit);
      mutateAndEvaluate(child, xDist(rng), yDist(rng), zDist(rng));
      if (i && penalizedFitness(child.value) > penalizedFitness(children[bestChild].value))
        bestChild = i;
    }
    auto &child(children[bestChild]);
    bool globalParetoChanged{};
    if (penalizedFitness(child.value) + 0.01 >= penalizedFitness(parent.value)) {
      if (penalizedFitness(child.value) > penalizedFitness(parent.value))
        nConverge = 0;
      parent = std::move(child);
      if (rawFitness(parent.value) > rawFitness(localUtopia)) {
        nConverge = 0;
        localUtopia = parent.value;
      }
      if (feasible(parent.value) && rawFitness(parent.value) > rawFitness(localPareto)) {
        nConverge = 0;
        localPareto = parent.value;
        if (rawFitness(parent.value) > rawFitness(globalPareto.value)) {
          globalPareto = parent;
          removeInvalidTiles();
          globalParetoChanged = true;
        }
      }
    }
    ++nConverge;
    return globalParetoChanged;
  }
}
