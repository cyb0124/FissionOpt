#include "OptMeta.h"

void OptMeta::restart() {
  population.front().state = xt::broadcast<int>(Tile::Air,
    {settings.sizeX, settings.sizeY, settings.sizeZ});
  population.front().value = evaluate(settings, population.front().state);
  for (int i(1); i < population.size(); ++i)
    population[i] = population[0];
}

OptMeta::OptMeta(const Settings &settings)
  :settings(settings) {
  restart();
  bestSoFar = population.front();
  for (int i{}; i < Tile::Air; ++i)
    if (settings.allowedCoolers[i])
      allTiles.emplace_back(i);
  allTiles.emplace_back(Tile::Air);
  allTiles.emplace_back(Tile::Cell);
  allTiles.emplace_back(Tile::Moderator);
}

bool OptMeta::step() {
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
    if (!bestIndividual || individual.value.effPower() > bestValue) {
      bestIndividual = i;
      bestValue = individual.value.effPower();
    }
  }
  if (bestValue + 0.01 >= population.front().value.effPower()) {
    std::swap(population.front(), population[bestIndividual]);
    if (bestValue > bestSoFar.value.effPower()) {
      bestSoFar = population.front();
      return true;
    }
  }
  return false;
}
