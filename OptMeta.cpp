#include "OptMeta.h"

void OptMeta::restart() {
  population.front().state = xt::broadcast<int>(Tile::Air,
    {settings.sizeX, settings.sizeY, settings.sizeZ});
  population.front().value = evaluate(settings, population.front().state);
  for (int i(1); i < population.size(); ++i)
    population[i] = population[0];
}

OptMeta::OptMeta(const Settings &settings)
  :settings(settings), nStagnation(),
  maxStagnation(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
  restart();
  best = population.front();
  bestNoNetHeat = population.front();
  for (int i{}; i < Tile::Air; ++i)
    if (settings.allowedCoolers[i])
      allTiles.emplace_back(i);
  allTiles.emplace_back(Tile::Air);
  allTiles.emplace_back(Tile::Cell);
  allTiles.emplace_back(Tile::Moderator);
}

int OptMeta::step() {
  if (nStagnation == maxStagnation)
    restart();
  int whichChanged{};
  int bestIndividual{};
  double bestValue;
  for (int i(1); i < population.size(); ++i) {
    auto &individual(population[i]);
    individual.state = population.front().state;
    int x(std::uniform_int_distribution<>(0, settings.sizeX - 1)(rng));
    int y(std::uniform_int_distribution<>(0, settings.sizeY - 1)(rng));
    int z(std::uniform_int_distribution<>(0, settings.sizeZ - 1)(rng));
    int tile(std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng));
    individual.state(x, y, z) = allTiles[tile];
    individual.value = evaluate(settings, individual.state);
    double value(individual.value.effPower());
    if (!bestIndividual || value > bestValue) {
      bestIndividual = i;
      bestValue = value;
    }
    if (value > best.value.effPower()) {
      best = individual;
      whichChanged |= 1;
    }
    if (individual.value.netHeat() <= 0.0 && value > bestNoNetHeat.value.effPower()) {
      bestNoNetHeat = individual;
      whichChanged |= 2;
    }
  }
  double oldValue(population.front().value.effPower());
  if (bestValue > oldValue)
    nStagnation = 0;
  if (bestValue + 0.01 >= oldValue)
    std::swap(population.front(), population[bestIndividual]);
  ++nStagnation;
  return whichChanged;
}
