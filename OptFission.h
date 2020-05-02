#ifndef _OPT_FISSION_H_
#define _OPT_FISSION_H_
#include <set>
#include <random>
#include "Fission.h"

namespace Fission {
  struct Sample {
    int limit[Air];
    xt::xtensor<int, 3> state;
    Evaluation value;
  };

  struct SampleComparator {
    const Settings &settings;
    bool operator()(const Sample &x, const Sample &y) const;
  };

  class Opt {
    const Settings &settings;
    int nStagnation, maxStagnation;
    std::multiset<Sample, SampleComparator> samples;
    Sample best;
    std::mt19937 rng;
    void restart();
  public:
    Opt(const Settings &settings);
    bool step();
    const Sample &getBest() const { return best; }
  };
}

#endif
