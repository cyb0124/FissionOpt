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
    Evaluator evaluator;
    Coords allowedCoords;
    std::vector<int> allowedTiles;
    int nEpisode, nStage, nIteration;
    int nConverge, maxConverge;
    double infeasibilityPenalty;
    Sample parent, best;
    std::array<Sample, 4> children;
    std::mt19937 rng;
    void restart();
    bool feasible(const Evaluation &x);
    double rawFitness(const Evaluation &x);
    double penalizedFitness(const Evaluation &x);
    int getNSym(int x, int y, int z);
    void setTileWithSym(Sample &sample, int x, int y, int z, int tile);
    void mutateAndEvaluate(Sample &sample, int x, int y, int z);
  public:
    Opt(const Settings &settings);
    bool step();
    bool stepBatch(int nBatch);
    const Sample &getBest() const { return best; }
    int getNEpisode() const { return nEpisode; }
    int getNStage() const { return nStage; }
    int getNIteration() const { return nIteration; }
  };
}

#endif
