#ifndef _FISSION_NET_H_
#define _FISSION_NET_H_
#include <unordered_map>
#include "OptFission.h"

namespace Fission {
  constexpr int nHidden(256), nMiniBatch(32);
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
  public:
    Net(Opt &opt);
    int getNFeatures() const { return nFeatures; }
    xt::xtensor<double, 1> assembleInput(const Sample &sample);
    double infer(const xt::xtensor<double, 1> &vInput);
    double train(const xt::xtensor<double, 2> &vInput, const xt::xtensor<double, 1> &vTarget);
  };
}

#endif
