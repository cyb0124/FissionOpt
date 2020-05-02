#include "OptFission.h"

namespace Fission {
  bool SampleComparator::operator()(const Sample &x, const Sample &y) const {
    return x.value.betterThan(settings.goals, y.value);
  }

  void Opt::restart() {
    std::vector<std::tuple<int, int, int>> positions;
    for (int x{}; x < settings.sizeX; ++x)
      for (int y{}; y < settings.sizeY; ++y)
        for (int z{}; z < settings.sizeZ; ++z)
          positions.emplace_back(x, y, z);
    for (int i{}; i < 1024; ++i) {
      Sample sample;
      std::copy(settings.limit, settings.limit + Air, sample.limit);
      sample.state = xt::broadcast<int>(Air,
        {settings.sizeX, settings.sizeY, settings.sizeZ});
      std::shuffle(positions.begin(), positions.end(), rng);
      std::vector<int> allTiles;
      allTiles.clear();
      for (auto &[x, y, z] : positions) {
        for (int tile{}; tile < Air; ++tile)
          if (sample.limit[tile])
            allTiles.emplace_back(tile);
        if (allTiles.empty())
          break;
        int newTile(allTiles[std::uniform_int_distribution<>(0, static_cast<int>(allTiles.size() - 1))(rng)]);
        --sample.limit[newTile];
        sample.state(x, y, z) = newTile;
      }
      sample.value = evaluate(settings, sample.state);
      samples.emplace(sample);
    }
  }

  Opt::Opt(const Settings &settings)
    :settings(settings), nStagnation(),
    maxStagnation(settings.sizeX * settings.sizeY * settings.sizeZ * 16),
    samples(SampleComparator{settings}) {
    restart();
    best = *samples.begin();
  }

  bool Opt::step() {
    if (nStagnation == maxStagnation)
      restart();
    std::uniform_int_distribution<> distIndex(0, static_cast<int>(samples.size() - 1));
    Sample sample(*std::next(samples.begin(), std::min(distIndex(rng), distIndex(rng))));
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
    sample.value = evaluate(settings, sample.state);
    samples.erase(std::prev(samples.end()));
    auto itr(samples.emplace(sample));
    bool bestChanged{};
    if (itr == samples.begin()) {
      nStagnation = 0;
      if (sample.value.betterThan(settings.goals, best.value)) {
        best = std::move(sample);
        bestChanged = true;
      }
    }
    ++nStagnation;
    return bestChanged;
  }
}
