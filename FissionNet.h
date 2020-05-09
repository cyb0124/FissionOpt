#ifndef _FISSION_NET_H_
#define _FISSION_NET_H_
#include <unordered_map>
#include "OptFission.h"

namespace Fission {
  constexpr int nHidden(256), nMiniBatch(32), nData(1'000'000);
  constexpr double lRate(0.001), mRate(0.9), rRate(0.999);

  class Net {
    Opt &opt;
    double mCorrector, rCorrector;
    std::unordered_map<int, int> tileMap;
    int nFeatures;
    xt::xtensor<double, 2> wHidden, gwHidden, mwHidden, rwHidden;
    xt::xtensor<double, 1> bHidden, gbHidden, mbHidden, rbHidden;
    xt::xtensor<double, 1> wOutput, gwOutput, mwOutput, rwOutput;
    double bOutput, gbOutput, mbOutput, rbOutput;

    // Data Pool
    xt::xtensor<double, 2> batchInput;
    xt::xtensor<double, 1> batchTarget;
    std::vector<std::pair<xt::xtensor<double, 1>, double>> data;
    int trajectoryLength, writePos;

    xt::xtensor<double, 1> extractFeatures(const Sample &sample);
  public:
    Net(Opt &opt);
    double infer(const Sample &sample);
    void newTrajectory() { trajectoryLength = 0; }
    void appendTrajectory(const Sample &sample);
    void finishTrajectory(double target);
    int getTrajectoryLength() const { return trajectoryLength; }
    double train();
  };
}

#endif
