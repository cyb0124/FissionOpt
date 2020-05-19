#ifndef _OPT_FISSION_H_
#define _OPT_FISSION_H_
#include <random>
#include <memory>
#include "Fission.h"

namespace Fission {
  struct Sample {
    int limit[Air];
    xt::xtensor<int, 3> state;
    Evaluation value;
  };

  enum {
    StageTrain = -2,
    StageInfer
  };

  constexpr int interactiveMin(1024), interactiveScale(327680), interactiveNet(16), nLossHistory(256);

  class Net;

  class Opt {
    friend Net;
    const Settings &settings;
    Evaluator evaluator;
    Coords allowedCoords;
    std::vector<int> allowedTiles;
    int nEpisode, nStage, nIteration;
    int nConverge, maxConverge;
    double infeasibilityPenalty;
    double parentFitness;
    Sample parent, best;
    std::array<Sample, 4> children;
    std::mt19937 rng;
    std::unique_ptr<Net> net;
    bool inferenceFailed;
    bool bestChanged;
    int redrawNagle;
    std::vector<double> lossHistory;
    bool lossChanged;
    void restart();
    bool feasible(const Evaluation &x);
    double rawFitness(const Evaluation &x);
    double currentFitness(const Sample &x);
    int getNSym(int x, int y, int z);
    void setTileWithSym(Sample &sample, int x, int y, int z, int tile);
    void mutateAndEvaluate(Sample &sample, int x, int y, int z);
  public:
    Opt(const Settings &settings, bool useNet);
    void step();
    void stepInteractive();
    bool needsRedrawBest();
    bool needsReplotLoss();
    const std::vector<double> &getLossHistory() const { return lossHistory; }
    const Sample &getBest() const { return best; }
    int getNEpisode() const { return nEpisode; }
    int getNStage() const { return nStage; }
    int getNIteration() const { return nIteration; }
  };
}

#endif
