#include "OptFission.h"

namespace Fission {
  void Opt::restart() {
    std::copy(settings.limit, settings.limit + Air, parent.limit);
    parent.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    parent.value = evaluate(settings, parent.state, nullptr);
    localUtopia = parent.value;
    localPareto = parent.value;
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), nConverge(),
    maxConverge(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
    restart();
    globalPareto = parent;
  }

  void Opt::removeInvalidTiles() {
    std::vector<std::tuple<int, int, int>> invalidTiles;
    evaluate(settings, globalPareto.state, &invalidTiles);
    for (auto [x, y, z] : invalidTiles)
      globalPareto.state(x, y, z) = Air;
  }

  double Opt::penalizedFitness(const Evaluation &x) {
    double result(x.fitness(settings));
    if (!x.feasible(settings))
      result -= (localUtopia.fitness(settings) - localPareto.fitness(settings)) * x.netHeat;
    return result;
  }

  bool Opt::step() {
    if (nConverge == maxConverge)
      restart();
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
      int x(xDist(rng)), y(yDist(rng)), z(zDist(rng));
      int oldTile(child.state(x, y, z));
      if (oldTile != Air)
        ++child.limit[oldTile];
      std::vector<int> allTiles{Air};
      for (int tile{}; tile < Air; ++tile)
        if (child.limit[tile])
          allTiles.emplace_back(tile);
      int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
      if (newTile != Air)
        --child.limit[newTile];
      child.state(x, y, z) = newTile;
      child.value = evaluate(settings, child.state, nullptr);
      if (i && penalizedFitness(child.value) > penalizedFitness(children[bestChild].value))
        bestChild = i;
    }
    auto &child(children[bestChild]);
    bool globalParetoChanged{};
    if (penalizedFitness(child.value) + 0.01 >= penalizedFitness(parent.value)) {
      parent = std::move(child);
      if (parent.value.fitness(settings) > localUtopia.fitness(settings))
        localUtopia = parent.value;
      if (parent.value.feasible(settings) && parent.value.fitness(settings) > localPareto.fitness(settings)) {
        nConverge = 0;
        localPareto = parent.value;
        if (parent.value.fitness(settings) > globalPareto.value.fitness(settings)) {
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
