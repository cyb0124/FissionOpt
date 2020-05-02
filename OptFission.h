#ifndef _OPT_FISSION_H_
#define _OPT_FISSION_H_
#include <array>
#include <random>
#include "Fission.h"

namespace Fission {
  struct Individual {
    int limit[Air];
    xt::xtensor<int, 3> state;
    Evaluation value;
  };

  class Opt {
    const Settings &settings;
    int nStagnation, maxStagnation;
    Individual parent, best;
    std::mt19937 rng;
    void restart();
  public:
    Opt(const Settings &settings);
    bool step();
    const Individual &getBest() const { return best; }
  };
}

#endif
