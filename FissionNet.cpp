#include <xtensor/xrandom.hpp>
#include "FissionNet.h"

namespace Fission {
  Net::Net(Opt &opt) :opt(opt), mCorrector(1), rCorrector(1), tileMap{{Air, 0}}, trajectoryLength(), writePos() {
    for (int i{}; i < Air; ++i)
      if (opt.settings.limit[i])
        tileMap.emplace(i, tileMap.size());
    nFeatures = static_cast<int>(5 + tileMap.size() * 2);

    wLayer1 = xt::random::randn({nLayer1, nFeatures}, 0.0, 1.0 / std::sqrt(nFeatures), opt.rng);
    mwLayer1 = xt::zeros_like(wLayer1);
    rwLayer1 = xt::zeros_like(wLayer1);
    bLayer1 = xt::zeros<double>({nLayer1});
    mbLayer1 = xt::zeros_like(bLayer1);
    rbLayer1 = xt::zeros_like(bLayer1);

    wLayer2 = xt::random::randn({nLayer2, nLayer1}, 0.0, 1.0 / std::sqrt(nLayer1), opt.rng);
    mwLayer2 = xt::zeros_like(wLayer2);
    rwLayer2 = xt::zeros_like(wLayer2);
    bLayer2 = xt::zeros<double>({nLayer2});
    mbLayer2 = xt::zeros_like(bLayer2);
    rbLayer2 = xt::zeros_like(bLayer2);

    wOutput = xt::random::randn({nLayer2}, 0.0, 1.0 / std::sqrt(nLayer2), opt.rng);
    mwOutput = xt::zeros_like(wOutput);
    rwOutput = xt::zeros_like(wOutput);
    bOutput = 0.0;
    mbOutput = 0.0;
    rbOutput = 0.0;

    batchInput = xt::empty<double>({nMiniBatch, nFeatures});
    batchTarget = xt::empty<double>({nMiniBatch});
  }

  void Net::appendTrajectory(const Sample &sample) {
    ++trajectoryLength;
    if (data.size() == nData)
      data[writePos].first = extractFeatures(sample);
    else
      data.emplace_back(extractFeatures(sample), 0.0);
    if (++writePos == nData)
      writePos = 0;
  }

  void Net::finishTrajectory(double target) {
    int pos(writePos);
    for (int i{}; i < trajectoryLength; ++i) {
      if (--pos < 0)
        pos = nData - 1;
      data[pos].second = target;
    }
  }

  xt::xtensor<double, 1> Net::extractFeatures(const Sample &sample) {
    xt::xtensor<double, 1> vInput(xt::zeros<double>({nFeatures}));
    for (int x{}; x < opt.settings.sizeX; ++x)
      for (int y{}; y < opt.settings.sizeY; ++y)
        for (int z{}; z < opt.settings.sizeZ; ++z)
          ++vInput[tileMap[sample.state(x, y, z)]];
    for (auto &[x, y, z] : sample.value.invalidTiles)
      ++vInput[tileMap.size() + tileMap[sample.state(x, y, z)]];
    vInput.periodic(-1) = sample.value.powerMult;
    vInput.periodic(-2) = sample.value.heatMult;
    vInput.periodic(-3) = sample.value.cooling / opt.settings.fuelBaseHeat;
    vInput /= opt.settings.sizeX * opt.settings.sizeY * opt.settings.sizeZ;
    vInput.periodic(-4) = sample.value.dutyCycle;
    vInput.periodic(-5) = sample.value.efficiency;
    return vInput;
  }

  double Net::infer(const Sample &sample) {
    auto vInput(extractFeatures(sample));
    xt::xtensor<double, 1> vLayer1(bLayer1 + xt::sum(wLayer1 * vInput, -1));
    xt::xtensor<double, 1> vPwlLayer1(vLayer1 * 0.1 + xt::clip(vLayer1, -1.0, 1.0));
    xt::xtensor<double, 1> vLayer2(bLayer2 + xt::sum(wLayer2 * vPwlLayer1, -1));
    xt::xtensor<double, 1> vPwlLayer2(vLayer2 * 0.1 + xt::clip(vLayer2, -1.0, 1.0));
    return bOutput + xt::sum(wOutput * vPwlLayer2)();
  }

  double Net::train() {
    // Assemble batch
    std::uniform_int_distribution<size_t> dist(0, data.size() - 1);
    for (int i{}; i < nMiniBatch; ++i) {
      auto &sample(data[dist(opt.rng)]);
      xt::view(batchInput, i, xt::all()) = sample.first;
      batchTarget(i) = sample.second;
    }

    // Forward
    xt::xtensor<double, 2> vLayer1(bLayer1 + xt::sum(wLayer1 * xt::view(batchInput, xt::all(), xt::newaxis(), xt::all()), -1));
    xt::xtensor<double, 2> vPwlLayer1(vLayer1 * 0.1 + xt::clip(vLayer1, -1.0, 1.0));
    xt::xtensor<double, 2> vLayer2(bLayer2 + xt::sum(wLayer2 * xt::view(vPwlLayer1, xt::all(), xt::newaxis(), xt::all()), -1));
    xt::xtensor<double, 2> vPwlLayer2(vLayer2 * 0.1 + xt::clip(vLayer2, -1.0, 1.0));
    xt::xtensor<double, 1> vOutput(bOutput + xt::sum(wOutput * vPwlLayer2, -1));
    xt::xtensor<double, 1> losses(xt::square(vOutput - batchTarget));
    double loss(xt::mean(losses)());

    // Backward
    xt::xtensor<double, 1> gvOutput((vOutput - batchTarget) * 2 / nMiniBatch);
    double gbOutput(xt::sum(gvOutput)());
    xt::xtensor<double, 1> gwOutput(xt::sum(xt::view(gvOutput, xt::all(), xt::newaxis()) * vPwlLayer2, 0));
    xt::xtensor<double, 2> gvPwlLayer2(xt::empty_like(vPwlLayer2));
    for (int i{}; i < nMiniBatch; ++i)
      for (int j{}; j < nLayer2; ++j)
        gvPwlLayer2(i, j) = gvOutput(i) * wOutput(j);
    xt::xtensor<double, 2> gvLayer2(gvPwlLayer2 * (0.1 + (xt::abs(vLayer2) < 1.0)));
    xt::xtensor<double, 1> gbLayer2(xt::sum(gvLayer2, 0));
    xt::xtensor<double, 2> gwLayer2(xt::empty_like(wLayer2));
    for (int i{}; i < nLayer2; ++i)
      for (int j{}; j < nLayer1; ++j)
        gwLayer2(i, j) = xt::sum(xt::view(gvLayer2, xt::all(), i) * xt::view(vPwlLayer1, xt::all(), j))();
    xt::xtensor<double, 2> gvPwlLayer1(xt::sum(xt::view(gvLayer2, xt::all(), xt::all(), xt::newaxis()) * wLayer2, -2));
    xt::xtensor<double, 2> gvLayer1(gvPwlLayer1 * (0.1 + (xt::abs(vLayer1) < 1.0)));
    xt::xtensor<double, 1> gbLayer1(xt::sum(gvLayer1, 0));
    xt::xtensor<double, 2> gwLayer1(xt::empty_like(wLayer1));
    for (int i{}; i < nLayer1; ++i)
      for (int j{}; j < nFeatures; ++j)
        gwLayer1(i, j) = xt::sum(xt::view(gvLayer1, xt::all(), i) * xt::view(batchInput, xt::all(), j))();

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

    wLayer1 -= mwLayer1 / ((1 - mCorrector) * (xt::sqrt(rwLayer1 / (1 - rCorrector)) + 1e-8));
    bLayer1 -= mbLayer1 / ((1 - mCorrector) * (xt::sqrt(rbLayer1 / (1 - rCorrector)) + 1e-8));
    wLayer2 -= mwLayer2 / ((1 - mCorrector) * (xt::sqrt(rwLayer2 / (1 - rCorrector)) + 1e-8));
    bLayer2 -= mbLayer2 / ((1 - mCorrector) * (xt::sqrt(rbLayer2 / (1 - rCorrector)) + 1e-8));
    wOutput -= mwOutput / ((1 - mCorrector) * (xt::sqrt(rwOutput / (1 - rCorrector)) + 1e-8));
    bOutput -= mbOutput / ((1 - mCorrector) * (std::sqrt(rbOutput / (1 - rCorrector)) + 1e-8));

    return loss;
  }
}
