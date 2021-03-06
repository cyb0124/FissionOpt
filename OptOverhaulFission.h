#ifndef _OPT_OVERHAUL_FISSION_H_
#define _OPT_OVERHAUL_FISSION_H_
#include <random>
#include "OverhaulFission.h"

namespace OverhaulFission {
  struct Sample {
    int limits[Tiles::Air];
    int sourceLimits[3];
    std::vector<int> cellLimits;
    xt::xtensor<int, 3> state;
    Evaluation value, valueWithShield;
  };

  enum {
    StageRollout,
    StageTrain,
    StageInfer
  };

  constexpr int interactiveMin(4096), interactiveScale(327680), interactiveNet(4), nLossHistory(256);
  constexpr int maxConvergeInfer(10976), maxConvergeRollout(maxConvergeInfer * 100), nConstraints(2), penaltyUpdatePeriod(maxConvergeInfer);

  class Net;

  class Opt {
    friend Net;
    std::mt19937 rng;
    const Settings &settings;
    std::vector<Coord> allowedCoords;
    std::vector<int> allowedTiles;
    int nEpisode, nStage, nIteration;
    int nConverge;
    xt::xtensor<bool, 1> hasFeasible, hasInfeasible;
    xt::xtensor<double, 1> penalty;
    std::vector<xt::xtensor<double, 1>> trajectoryBuffer;
    double parentFitness, localBest;
    Sample parent, child, best;
    std::unique_ptr<Net> net;
    bool inferenceFailed;
    bool bestChanged;
    int redrawNagle;
    std::vector<double> lossHistory;
    bool lossChanged;
    void restart();
    xt::xtensor<bool, 1> feasible(const Sample &x);
    xt::xtensor<double, 1> infeasibility(const Sample &x);
    double rawFitness(const Evaluation &x);
    double currentFitness(const Sample &x);
    int getNSym(int x, int y, int z);
    void setTileWithSym(Sample &sample, int x, int y, int z, int tile);
    void mutateAndEvaluate(Sample &sample, int x, int y, int z);
  public:
    Opt(Settings &settings);
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
