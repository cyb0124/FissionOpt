#ifndef _FISSION_NET_H_
#define _FISSION_NET_H_
#include <xtensor/xview.hpp>
#include <unordered_map>
#include "OptFission.h"

namespace Fission {
  constexpr int nLayer1(128), nLayer2(64);
  constexpr int nMiniBatch(64), nEpoch(2), nPool(1'000'000);
  constexpr double lRate(0.01), mRate(0.9), rRate(0.999);

  class Net {
    std::mt19937 &rng;
    int nFeatures;

    // Data Pool
    xt::xtensor<double, 2> batchInput;
    xt::xtensor<double, 1> batchTarget;
    std::vector<std::pair<xt::xtensor<double, 1>, double>> pool;
    int trajectoryLength, writePos;

    // Model
    int nLastBatch;
    double mCorrector, rCorrector;
    // nLayer1 * nFeatures
    xt::xtensor<double, 2> wLayer1, mwLayer1, rwLayer1;
    // nLayer1
    xt::xtensor<double, 1> bLayer1, mbLayer1, rbLayer1;
    // nLayer2 * nLayer1
    xt::xtensor<double, 2> wLayer2, mwLayer2, rwLayer2;
    // nLayer2
    xt::xtensor<double, 1> bLayer2, mbLayer2, rbLayer2;
    // nLayer2
    xt::xtensor<double, 1> wOutput, mwOutput, rwOutput;
    double bOutput, mbOutput, rbOutput;

    // Intermediates
    // nBatch * nLayer1
    xt::xtensor<double, 2> vLayer1Pre, vLayer1Post;
    // nBatch * nLayer2
    xt::xtensor<double, 2> vLayer2Pre, vLayer2Post;
    // nBatch
    xt::xtensor<double, 1> vOutput;

    template<typename T>
    void forward(const T &vInput) {
      if (vInput.shape(0) != nLastBatch) {
        nLastBatch = vInput.shape(0);
        vLayer1Pre = xt::empty<double>({nLastBatch, nLayer1});
        vLayer1Post = xt::empty_like(vLayer1Pre);
        vLayer2Pre = xt::empty<double>({nLastBatch, nLayer2});
        vLayer2Post = xt::empty_like(vLayer2Pre);
        vOutput = xt::empty<double>({nLastBatch});
      }
      vLayer1Pre = xt::sum(wLayer1 * xt::view(vInput, xt::all(), xt::newaxis(), xt::all()), 2) + bLayer1;
      vLayer1Post = xt::tanh(vLayer1Pre);
      vLayer2Pre = xt::sum(wLayer2 * xt::view(vLayer1Post, xt::all(), xt::newaxis(), xt::all()), 2) + bLayer2;
      vLayer2Post = xt::tanh(vLayer2Pre);
      vOutput = xt::sum(wOutput * vLayer2Post, 1) + bOutput;
    }
  public:
    Net(std::mt19937 &rng, Sample &anyValidSample);
    double infer(const Sample &sample);
    void newTrajectory() { trajectoryLength = 0; }
    void appendTrajectory(const Sample &sample);
    void finishTrajectory(double target);
    int getTrajectoryLength() const { return trajectoryLength; }
    double train();
  };
}

#endif
