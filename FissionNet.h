#ifndef _FISSION_NET_H_
#define _FISSION_NET_H_
#include "OptFission.h"

namespace Fission {
  constexpr int nHidden(64);
  constexpr double lRate(0.001), mRate(0.9), rRate(0.999);

  class Net {
    Opt &opt;
    int sizeX, sizeY, sizeZ;
    double mCorrector, rCorrector;
    xt::xtensor<double, 4> wEmbeddings, gwEmbeddings, mwEmbeddings, rwEmbeddings;
    xt::xtensor<double, 1> wRawFitness, gwRawFitness, mwRawFitness, rwRawFitness;
    xt::xtensor<double, 1> bHidden, gbHidden, mbHidden, rbHidden;
    xt::xtensor<double, 1> vHidden, vPwlHidden;
    xt::xtensor<double, 1> wOutput, gwOutput, mwOutput, rwOutput;
    double bOutput, gbOutput, mbOutput, rbOutput;
    double vOutput;
  public:
    Net(Opt &opt);
    double forward(const Sample &sample);
    void backward(double g, const Sample &sample);
    void adam();
  };
}

#endif
