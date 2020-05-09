#include <xtensor/xrandom.hpp>
#include "FissionNet.h"

namespace Fission {
  Net::Net(Opt &opt) :opt(opt), mCorrector(1), rCorrector(1), tileMap{{Air, 0}}, trajectoryLength(), writePos() {
    for (int i{}; i < Air; ++i)
      if (opt.settings.limit[i])
        tileMap.emplace(i, tileMap.size());
    nFeatures = 3 + tileMap.size() * 2;
    wHidden = xt::random::randn({nHidden, nFeatures}, 0.0, 1.0 / std::sqrt(nFeatures), opt.rng);
    gwHidden = xt::empty_like(wHidden);
    mwHidden = xt::zeros_like(wHidden);
    rwHidden = xt::zeros_like(wHidden);
    bHidden = xt::zeros<double>({nHidden});
    mbHidden = xt::zeros_like(bHidden);
    rbHidden = xt::zeros_like(bHidden);
    wOutput = xt::random::randn({nHidden}, 0.0, 1.0 / std::sqrt(nHidden), opt.rng);
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
    return vInput;
  }

  double Net::infer(const Sample &sample) {
    auto vInput(extractFeatures(sample));
    xt::xtensor<double, 1> vHidden(bHidden + xt::sum(wHidden * vInput, -1));
    xt::xtensor<double, 1> vPwlHidden(vHidden * 0.1 + xt::clip(vHidden, -1.0, 1.0));
    return bOutput + xt::sum(wOutput * vPwlHidden)();
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
    xt::xtensor<double, 2> vHidden(bHidden + xt::sum(wHidden * xt::view(batchInput, xt::all(), xt::newaxis(), xt::all()), -1));
    xt::xtensor<double, 2> vPwlHidden(vHidden * 0.1 + xt::clip(vHidden, -1.0, 1.0));
    xt::xtensor<double, 1> vOutput(bOutput + xt::sum(wOutput * vPwlHidden, -1));
    xt::xtensor<double, 1> losses(xt::square(vOutput - batchTarget));
    double loss(xt::mean(losses)());

    // Backward
    xt::xtensor<double, 1> gvOutput((vOutput - batchTarget) * 2 / nMiniBatch);
    double gbOutput(xt::sum(gvOutput)());
    xt::xtensor<double, 1> gwOutput(xt::sum(xt::view(gvOutput, xt::all(), xt::newaxis()) * vPwlHidden, 0));
    xt::xtensor<double, 2> gvPwlHidden(xt::empty_like(vPwlHidden));
    for (int i{}; i < nMiniBatch; ++i)
      for (int j{}; j < nHidden; ++j)
        gvPwlHidden(i, j) = gvOutput(i) * wOutput(j);
    xt::xtensor<double, 2> gvHidden(gvPwlHidden * (0.1 + (xt::abs(vHidden) < 1.0)));
    xt::xtensor<double, 1> gbHidden(xt::sum(gvHidden, 0));
    xt::xtensor<double, 2> gwHidden(xt::empty_like(wHidden));
    for (int i{}; i < nHidden; ++i)
      for (int j{}; j < nFeatures; ++j)
        gwHidden(i, j) = xt::sum(xt::view(gvHidden, xt::all(), i) * xt::view(batchInput, xt::all(), j))();

    // Adam
    mCorrector *= mRate;
    mwHidden = mRate * mwHidden + (1 - mRate) * gwHidden;
    mbHidden = mRate * mbHidden + (1 - mRate) * gbHidden;
    mwOutput = mRate * mwOutput + (1 - mRate) * gwOutput;
    mbOutput = mRate * mbOutput + (1 - mRate) * gbOutput;

    rCorrector *= rRate;
    rwHidden = rRate * rwHidden + (1 - rRate) * xt::square(gwHidden);
    rbHidden = rRate * rbHidden + (1 - rRate) * xt::square(gbHidden);
    rwOutput = rRate * rwOutput + (1 - rRate) * xt::square(gwOutput);
    rbOutput = rRate * rbOutput + (1 - rRate) * (gbOutput * gbOutput);

    wHidden -= mwHidden / ((1 - mCorrector) * (xt::sqrt(rwHidden / (1 - rCorrector)) + 1e-8));
    bHidden -= mbHidden / ((1 - mCorrector) * (xt::sqrt(rbHidden / (1 - rCorrector)) + 1e-8));
    wOutput -= mwOutput / ((1 - mCorrector) * (xt::sqrt(rwOutput / (1 - rCorrector)) + 1e-8));
    bOutput -= mbOutput / ((1 - mCorrector) * (std::sqrt(rbOutput / (1 - rCorrector)) + 1e-8));

    return loss;
  }
}
