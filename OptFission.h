#ifndef _OPT_FISSION_H_
#define _OPT_FISSION_H_
#include <random>
#include "Fission.h"

namespace Fission {
  struct Sample {
    int limit[Air];
    xt::xtensor<int, 3> state;
    Evaluation value;
  };

  class Opt {
    const Settings &settings;
    std::vector<std::tuple<int, int, int>> positions;
    double maxCooling;
    int nConverge, maxConverge;
    bool penaltyEnabled;
    Evaluation localUtopia, localPareto;
    Sample parent, globalPareto;
    std::mt19937 rng;
    void restart();
    void removeInvalidTiles();
    bool feasible(const Evaluation &x);
    double rawFitness(const Evaluation &x);
    double penalizedFitness(const Evaluation &x);
    int getNSym(int x, int y, int z);
    void setTileWithSym(Sample &sample, int x, int y, int z, int tile);
    void mutateAndEvaluate(Sample &sample, int x, int y, int z);
  public:
    Opt(const Settings &settings);
    bool step();
    const Sample &getBest() const { return globalPareto; }
  };
}

#endif
