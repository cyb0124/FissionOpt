#include <xtensor/xrandom.hpp>
#include "FissionNet.h"

namespace Fission {
  Net::Net(Opt &opt)
    :opt(opt),
    sizeX(opt.settings.symX ? (opt.settings.sizeX + 1) / 2 : opt.settings.sizeX),
    sizeY(opt.settings.symY ? (opt.settings.sizeY + 1) / 2 : opt.settings.sizeY),
    sizeZ(opt.settings.symZ ? (opt.settings.sizeZ + 1) / 2 : opt.settings.sizeZ),
    mCorrector(1), rCorrector(1),
    wEmbeddings(xt::random::randn({sizeX, sizeY, sizeZ, nHidden}, 0.0, 1.0 / std::sqrt(sizeX * sizeY * sizeZ), opt.rng)),
    gwEmbeddings(xt::empty_like(wEmbeddings)),
    mwEmbeddings(xt::zeros_like(wEmbeddings)),
    rwEmbeddings(xt::zeros_like(wEmbeddings)),
    wRawFitness(xt::random::randn({nHidden}, 0.0, 0.1, opt.rng)),
    mwRawFitness(xt::zeros_like(wRawFitness)),
    rwRawFitness(xt::zeros_like(wRawFitness)),
    bHidden(xt::zeros<double>({nHidden})),
    mbHidden(xt::zeros_like(bHidden)),
    rbHidden(xt::zeros_like(bHidden)),
    wOutput(xt::random::randn({nHidden}, 0.0, 1.0 / std::sqrt(nHidden), opt.rng)),
    mwOutput(xt::zeros_like(wOutput)),
    rwOutput(xt::zeros_like(wOutput)),
    bOutput(), mbOutput(), rbOutput() {
  }

  double Net::forward(const Sample &sample) {
    vHidden = bHidden
      + wRawFitness * opt.rawFitness(sample.value);
    for (int x{}; x < sizeX; ++x)
      for (int y{}; y < sizeY; ++y)
        for (int z{}; z < sizeZ; ++z)
          if (sample.state(x, y, z) == Cell)
            vHidden += xt::view(wEmbeddings, x, y, z, xt::all());
    vPwlHidden = vHidden * 0.1 + xt::clip(vHidden, -1.0, 1.0);
    vOutput = bOutput + xt::sum(wOutput * vPwlHidden)();
    return vOutput;
  }

  void Net::backward(double g, const Sample &sample) {
    gbOutput = g;
    gwOutput = g * vPwlHidden;
    auto gvPwlHidden(g * wOutput);
    auto gvHidden(gvPwlHidden * (0.1 + (xt::abs(vHidden) < 1.0)));
    for (int x{}; x < sizeX; ++x)
      for (int y{}; y < sizeY; ++y)
        for (int z{}; z < sizeZ; ++z)
          if (sample.state(x, y, z) == Cell)
            xt::view(gwEmbeddings, x, y, z, xt::all()) = gvHidden;
          else
            xt::view(gwEmbeddings, x, y, z, xt::all()) = 0.0;
    gwRawFitness = gvHidden * opt.rawFitness(sample.value);
    gbHidden = gvHidden;
  }

  void Net::adam() {
    mCorrector *= mRate;
    mwEmbeddings = mRate * mwEmbeddings + (1 - mRate) * gwEmbeddings;
    mwRawFitness = mRate * mwRawFitness + (1 - mRate) * gwRawFitness;
    mbHidden = mRate * mbHidden + (1 - mRate) * gbHidden;
    mwOutput = mRate * mwOutput + (1 - mRate) * gwOutput;
    mbOutput = mRate * mbOutput + (1 - mRate) * gbOutput;

    rCorrector *= rRate;
    rwEmbeddings = rRate * rwEmbeddings + (1 - rRate) * xt::square(gwEmbeddings);
    rwRawFitness = rRate * rwRawFitness + (1 - rRate) * xt::square(gwRawFitness);
    rbHidden = rRate * rbHidden + (1 - rRate) * xt::square(gbHidden);
    rwOutput = rRate * rwOutput + (1 - rRate) * xt::square(gwOutput);
    rbOutput = rRate * rbOutput + (1 - rRate) * (gbOutput * gbOutput);

    wEmbeddings -= mwEmbeddings / ((1 - mCorrector) * (xt::sqrt(rwEmbeddings / (1 - rCorrector)) + 1e-8));
    wRawFitness -= mwRawFitness / ((1 - mCorrector) * (xt::sqrt(rwRawFitness / (1 - rCorrector)) + 1e-8));
    bHidden -= mbHidden / ((1 - mCorrector) * (xt::sqrt(rbHidden / (1 - rCorrector)) + 1e-8));
    wOutput -= mwOutput / ((1 - mCorrector) * (xt::sqrt(rwOutput / (1 - rCorrector)) + 1e-8));
    bOutput -= mbOutput / ((1 - mCorrector) * (std::sqrt(rbOutput / (1 - rCorrector)) + 1e-8));
  }
}
