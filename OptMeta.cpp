#include "OptMeta.h"

void OptMeta::restart() {
  parent.state = xt::broadcast<int>(Tile::Air,
    {settings.sizeX, settings.sizeY, settings.sizeZ});
  parent.value = evaluate(settings, parent.state);
  std::copy(settings.limit, settings.limit + Tile::Air, parent.limit);
}

OptMeta::OptMeta(const Settings &settings)
  :settings(settings), nStagnation(),
  maxStagnation(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
  restart();
  best = parent;
  bestNoNetHeat = parent;
}

int OptMeta::step() {
  if (nStagnation == maxStagnation)
    restart();
  int whichChanged{};
  int bestChild{};
  double bestValue;
  std::array<OptMetaIndividual, 4> children;
  for (int i{}; i < children.size(); ++i) {
    auto &child(children[i]);
    child.state = parent.state;
    std::copy(parent.limit, parent.limit + Tile::Air, child.limit);
    int x(std::uniform_int_distribution<>(0, settings.sizeX - 1)(rng));
    int y(std::uniform_int_distribution<>(0, settings.sizeY - 1)(rng));
    int z(std::uniform_int_distribution<>(0, settings.sizeZ - 1)(rng));
    int oldTile(child.state(x, y, z));
    if (oldTile != Tile::Air)
      ++child.limit[oldTile];
    std::vector<int> allTiles{Tile::Air};
    for (int tile{}; tile < Tile::Air; ++tile)
      if (child.limit[tile])
        allTiles.emplace_back(tile);
    int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
    if (newTile != Tile::Air)
      --child.limit[newTile];
    child.state(x, y, z) = newTile;
    child.value = evaluate(settings, child.state);
    double value(child.value.effPower());
    if (!bestChild || value > bestValue) {
      bestChild = i;
      bestValue = value;
    }
    if (value > best.value.effPower()) {
      best = child;
      whichChanged |= 1;
    }
    if (child.value.netHeat() <= 0.0 && value > bestNoNetHeat.value.effPower()) {
      bestNoNetHeat = child;
      whichChanged |= 2;
    }
  }
  double oldValue(parent.value.effPower());
  if (bestValue > oldValue)
    nStagnation = 0;
  if (bestValue + 0.01 >= oldValue)
    parent = std::move(children[bestChild]);
  ++nStagnation;
  return whichChanged;
}
