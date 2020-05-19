#include <xtensor/xrandom.hpp>
#include "FissionNet.h"

namespace Fission {
  Net::Net(std::mt19937 &rng, Sample &anyValidSample)
    :rng(rng), nFeatures(anyValidSample.value.features.size()),
    trajectoryLength(), writePos(), nLastBatch(), mCorrector(1), rCorrector(1) {
    batchInput = xt::empty<double>({nMiniBatch, nFeatures});
    batchTarget = xt::empty<double>({nMiniBatch});

    wLayer1 = xt::random::randn({nLayer1, nFeatures}, 0.0, 1.0 / std::sqrt(nFeatures), rng);
    mwLayer1 = xt::zeros_like(wLayer1);
    rwLayer1 = xt::zeros_like(wLayer1);

    bLayer1 = xt::zeros<double>({nLayer1});
    mbLayer1 = xt::zeros_like(bLayer1);
    rbLayer1 = xt::zeros_like(bLayer1);

    wLayer2 = xt::random::randn({nLayer2, nLayer1}, 0.0, 1.0 / std::sqrt(nLayer1), rng);
    mwLayer2 = xt::zeros_like(wLayer2);
    rwLayer2 = xt::zeros_like(wLayer2);

    bLayer2 = xt::zeros<double>({nLayer2});
    mbLayer2 = xt::zeros_like(bLayer2);
    rbLayer2 = xt::zeros_like(bLayer2);

    wOutput = xt::random::randn({nLayer2}, 0.0, 1.0 / std::sqrt(nLayer2), rng);
    mwOutput = xt::zeros_like(wOutput);
    rwOutput = xt::zeros_like(wOutput);

    bOutput = 0.0;
    mbOutput = 0.0;
    rbOutput = 0.0;
  }

  void Net::appendTrajectory(const Sample &sample) {
    ++trajectoryLength;
    if (pool.size() == nPool)
      pool[writePos].first = xt::reshape_view(sample.value.features, {nFeatures});
    else
      pool.emplace_back(xt::reshape_view(sample.value.features, {nFeatures}), 0.0);
    if (++writePos == nPool)
      writePos = 0;
  }

  void Net::finishTrajectory(double target) {
    int pos(writePos);
    for (int i{}; i < trajectoryLength; ++i) {
      if (--pos < 0)
        pos = nPool - 1;
      pool[pos].second = target;
    }
  }

  double Net::infer(const Sample &sample) {
    forward(xt::reshape_view(sample.value.features, {1, nFeatures}));
    return vOutput(0);
  }

  double Net::train() {
    // Assemble batch
    std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
    for (int i{}; i < nMiniBatch; ++i) {
      auto &sample(pool[dist(rng)]);
      xt::view(batchInput, i, xt::all()) = sample.first;
      batchTarget(i) = sample.second;
    }

    // Forward
    forward(batchInput);
    xt::xtensor<double, 1> vErrors(vOutput - batchTarget);
    double loss(xt::mean(xt::square(vErrors))());

    // Backprop
    xt::xtensor<double, 1> gvOutput(vErrors * 2 / nMiniBatch);
    double gbOutput(xt::sum(gvOutput)());
    xt::xtensor<double, 1> gwOutput(xt::sum(xt::view(gvOutput, xt::all(), xt::newaxis()) * vLayer2Post, 0));
    xt::xtensor<double, 2> gvLayer2Post(xt::empty_like(vLayer2Post));
    for (int i{}; i < nMiniBatch; ++i)
      for (int j{}; j < nLayer2; ++j)
        gvLayer2Post(i, j) = gvOutput(i) * wOutput(j);
    xt::xtensor<double, 2> gvLayer2Pre(gvLayer2Post * (1 - xt::square(vLayer2Post)));
    xt::xtensor<double, 1> gbLayer2(xt::sum(gvLayer2Pre, 0));
    xt::xtensor<double, 2> gwLayer2(xt::empty_like(wLayer2));
    for (int i{}; i < nLayer2; ++i)
      for (int j{}; j < nLayer1; ++j)
        gwLayer2(i, j) = xt::sum(xt::view(gvLayer2Pre, xt::all(), i) * xt::view(vLayer1Post, xt::all(), j))();
    xt::xtensor<double, 2> gvLayer1Post(xt::sum(
      xt::view(vLayer2Pre, xt::all(), xt::all(), xt::newaxis()) * wLayer2, 1));
    xt::xtensor<double, 2> gvLayer1Pre(gvLayer1Post * (1 - xt::square(vLayer1Post)));
    xt::xtensor<double, 1> gbLayer1(xt::sum(gvLayer1Pre, 0));
    xt::xtensor<double, 2> gwLayer1(xt::empty_like(wLayer1));
    for (int i{}; i < nLayer1; ++i)
      for (int j{}; j < nFeatures; ++j)
        gwLayer1(i, j) = xt::sum(xt::view(gvLayer1Pre, xt::all(), i) * xt::view(batchInput, xt::all(), j))();

    // Adam
    mCorrector *= mRate;
    mwLayer1 = mRate * mwLayer1 + (1 - mRate) * gwLayer1;
    mbLayer1 = mRate * mbLayer1 + (1 - mRate) * gbLayer1;
    mwLayer2 = mRate * mwLayer2 + (1 - mRate) * gwLayer2;
    mbLayer2 = mRate * mbLayer2 + (1 - mRate) * gbLayer2;
    mwOutput = mRate * mwOutput + (1 - mRate) * gwOutput;
    mbOutput = mRate * mbOutput + (1 - mRate) * gbOutput;

    rCorrector *= rRate;
    rwLayer1 = rRate * rwLayer1 + (1 - rRate) * xt::square(gwLayer1);
    rbLayer1 = rRate * rbLayer1 + (1 - rRate) * xt::square(gbLayer1);
    rwLayer2 = rRate * rwLayer2 + (1 - rRate) * xt::square(gwLayer2);
    rbLayer2 = rRate * rbLayer2 + (1 - rRate) * xt::square(gbLayer2);
    rwOutput = rRate * rwOutput + (1 - rRate) * xt::square(gwOutput);
    rbOutput = rRate * rbOutput + (1 - rRate) * (gbOutput * gbOutput);

    wLayer1 -= lRate * mwLayer1 / ((1 - mCorrector) * (xt::sqrt(rwLayer1 / (1 - rCorrector)) + 1e-8));
    bLayer1 -= lRate * mbLayer1 / ((1 - mCorrector) * (xt::sqrt(rbLayer1 / (1 - rCorrector)) + 1e-8));
    wLayer2 -= lRate * mwLayer2 / ((1 - mCorrector) * (xt::sqrt(rwLayer2 / (1 - rCorrector)) + 1e-8));
    bLayer2 -= lRate * mbLayer2 / ((1 - mCorrector) * (xt::sqrt(rbLayer2 / (1 - rCorrector)) + 1e-8));
    wOutput -= lRate * mwOutput / ((1 - mCorrector) * (xt::sqrt(rwOutput / (1 - rCorrector)) + 1e-8));
    bOutput -= lRate * mbOutput / ((1 - mCorrector) * (std::sqrt(rbOutput / (1 - rCorrector)) + 1e-8));

    return loss;
  }
}
