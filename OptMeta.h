#ifndef _OPT_META_H_
#define _OPT_META_H_
#include <array>
#include <random>
#include "Fission.h"

struct OptMetaIndividual {
  xt::xtensor<int, 3> state;
  Evaluation value;
};

class OptMeta {
  const Settings &settings;
  int nStagnation, maxStagnation;
  std::array<OptMetaIndividual, 5> population;
  OptMetaIndividual best, bestNoNetHeat;
  std::vector<int> allTiles;
  std::mt19937 rng;
  void restart();
public:
  OptMeta(const Settings &settings);
  int step();
  const OptMetaIndividual &getBest() const { return best; }
  const OptMetaIndividual &getBestNoNetHeat() const { return bestNoNetHeat; }
};

#endif
