#include "OptFission.h"

namespace Fission {
  void Opt::restart() {
    auto &sample(samples.front());
    std::copy(settings.limit, settings.limit + Air, sample.limit);
    sample.state = xt::broadcast<int>(Air,
      {settings.sizeX, settings.sizeY, settings.sizeZ});
    sample.value = evaluate(settings, sample.state, nullptr);
    for (int i(1); i < samples.size(); ++i)
      samples[i] = sample;
    localUtopia = sample.value;
    localPareto = sample.value;
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), nConverge(),
    maxConverge(settings.sizeX * settings.sizeY * settings.sizeZ * 16) {
    restart();
    globalPareto = samples.front();
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
    std::uniform_int_distribution<> distIndex(0, static_cast<int>(samples.size() - 1));
    Sample &candidateA(samples[distIndex(rng)]), &candidateB(samples[distIndex(rng)]);
    Sample sample(penalizedFitness(candidateA.value) > penalizedFitness(candidateB.value) ? candidateA : candidateB);
    int x(std::uniform_int_distribution<>(0, settings.sizeX - 1)(rng));
    int y(std::uniform_int_distribution<>(0, settings.sizeY - 1)(rng));
    int z(std::uniform_int_distribution<>(0, settings.sizeZ - 1)(rng));
    int oldTile(sample.state(x, y, z));
    if (oldTile != Air)
      ++sample.limit[oldTile];
    std::vector<int> allTiles{Air};
    for (int tile{}; tile < Air; ++tile)
      if (sample.limit[tile])
        allTiles.emplace_back(tile);
    int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
    if (newTile != Air)
      --sample.limit[newTile];
    sample.state(x, y, z) = newTile;
    sample.value = evaluate(settings, sample.state, nullptr);
    auto range(std::minmax_element(samples.begin(), samples.end(), [&](const Sample &x, const Sample &y) {
      return penalizedFitness(x.value) < penalizedFitness(y.value);
    }));
    *range.first = sample;
    if (sample.value.fitness(settings) > localUtopia.fitness(settings))
      localUtopia = sample.value;
    bool globalParetoChanged{};
    if (sample.value.feasible(settings) && sample.value.fitness(settings) > localPareto.fitness(settings)) {
      nConverge = 0;
      localPareto = sample.value;
      if (sample.value.fitness(settings) > globalPareto.value.fitness(settings)) {
        globalPareto = std::move(sample);
        removeInvalidTiles();
        globalParetoChanged = true;
      }
    }
    ++nConverge;
    return globalParetoChanged;
  }
}
