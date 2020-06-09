#ifndef _OVERHAUL_FISSION_NET_H_
#define _OVERHAUL_FISSION_NET_H_
#include <unordered_map>
#include "OptOverhaulFission.h"

namespace OverhaulFission {
  constexpr int nStatisticalFeatures(8), nLayer1(128), nLayer2(64), nMiniBatch(64), nEpoch(2), nPool(10'000'000);
  constexpr double lRate(0.01), mRate(0.9), rRate(0.999), leak(0.1);

  class Net {
    Opt &opt;
    double mCorrector, rCorrector;
    std::unordered_map<int, int> tileMap;
    int nFeatures;

    // Data Pool
    xt::xtensor<double, 2> batchInput;
    xt::xtensor<double, 1> batchTarget;
    std::vector<std::pair<xt::xtensor<double, 1>, double>> pool;
    int trajectoryLength, writePos;

    xt::xtensor<double, 2> wLayer1, mwLayer1, rwLayer1;
    xt::xtensor<double, 1> bLayer1, mbLayer1, rbLayer1;
    xt::xtensor<double, 2> wLayer2, mwLayer2, rwLayer2;
    xt::xtensor<double, 1> bLayer2, mbLayer2, rbLayer2;
    xt::xtensor<double, 1> wOutput, mwOutput, rwOutput;
    double bOutput, mbOutput, rbOutput;

  public:
    Net(Opt &opt);
    xt::xtensor<double, 1> extractFeatures(const Sample &sample);
    double infer(const Sample &sample);
    void newTrajectory() { trajectoryLength = 0; }
    void appendTrajectory(xt::xtensor<double, 1> features);
    void finishTrajectory(double target);
    int getTrajectoryLength() const { return trajectoryLength; }
    double train();
  };
}

#endif
