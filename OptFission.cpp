#include "OptFission.h"

namespace Fission {
  void Opt::restart() {
    parent.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    parent.value = evaluate(settings, parent.state);
    std::copy(settings.limit, settings.limit + Air, parent.limit);
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), nStagnation(),
    maxStagnation(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
    restart();
    best = parent;
  }

  bool Opt::step() {
    if (nStagnation == maxStagnation)
      restart();
    int bestChild{};
    std::array<Individual, 4> children;
    for (int i{}; i < children.size(); ++i) {
      auto &child(children[i]);
      child.state = parent.state;
      std::copy(parent.limit, parent.limit + Air, child.limit);
      int x(std::uniform_int_distribution<>(0, settings.sizeX - 1)(rng));
      int y(std::uniform_int_distribution<>(0, settings.sizeY - 1)(rng));
      int z(std::uniform_int_distribution<>(0, settings.sizeZ - 1)(rng));
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
      child.value = evaluate(settings, child.state);
      if (!bestChild || child.value.betterThan(settings.goals, children[bestChild].value))
        bestChild = i;
    }
    bool bestChanged{};
    if (children[bestChild].value.asGoodAs(settings.goals, parent.value)) {
      if (children[bestChild].value.betterThan(settings.goals, parent.value))
        nStagnation = 0;
      parent = std::move(children[bestChild]);
      if (parent.value.betterThan(settings.goals, best.value)) {
        best = parent;
        bestChanged = true;
      }
    }
    ++nStagnation;
    return bestChanged;
  }
}
