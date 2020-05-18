#include <xtensor/xrandom.hpp>
#include "FissionNet.h"

namespace Fission {
  Net::Net(Opt &opt) :opt(opt),
    sizeX(opt.settings.symX ? (opt.settings.sizeX + 1) / 2 : opt.settings.sizeX),
    sizeY(opt.settings.symY ? (opt.settings.sizeY + 1) / 2 : opt.settings.sizeY),
    sizeZ(opt.settings.symZ ? (opt.settings.sizeZ + 1) / 2 : opt.settings.sizeZ),
    nBatch(), mCorrector(1), rCorrector(1), trajectoryLength(), writePos() {
    for (int i{}; i < Air; ++i)
      if (opt.settings.limit[i])
        tileMap.emplace(i, tileMap.size());
    tileMap.emplace(Air, tileMap.size());

    wEmbeddings = xt::random::randn({static_cast<int>(tileMap.size()), nChannels}, 0.0, 1.0, opt.rng);
    mwEmbeddings = xt::zeros_like(wEmbeddings);
    rwEmbeddings = xt::zeros_like(wEmbeddings);

    wConvs = xt::random::randn({nConvs, 3, 3, 3, nChannels, nChannels}, 0.0, 1.0 / std::sqrt(3 * 3 * 3 * nChannels), opt.rng);
    mwConvs = xt::zeros_like(wConvs);
    rwConvs = xt::zeros_like(wConvs);

    bConvs = xt::zeros<double>({nConvs, nChannels});
    mbConvs = xt::zeros_like(bConvs);
    rbConvs = xt::zeros_like(bConvs);

    wLocal = xt::random::randn({nChannels}, 0.0, 1.0 / std::sqrt(nChannels), opt.rng);
    mwLocal = xt::zeros_like(wLocal);
    rwLocal = xt::zeros_like(wLocal);

    bLocal = 0.0;
    mbLocal = 0.0;
    rbLocal = 0.0;

    wGlobal = xt::random::randn({nFeatures, sizeX, sizeY, sizeZ}, 0.0, 1.0 / std::sqrt(sizeX * sizeY * sizeZ), opt.rng);
    mwGlobal = xt::zeros_like(wGlobal);
    rwGlobal = xt::zeros_like(wGlobal);

    bGlobal = xt::zeros<double>({nFeatures});
    mbGlobal = xt::zeros_like(bGlobal);
    rbGlobal = xt::zeros_like(bGlobal);

    wOutput = xt::random::randn({nFeatures}, 0.0, 1.0 / std::sqrt(nFeatures), opt.rng);
    mwOutput = xt::zeros_like(wOutput);
    rwOutput = xt::zeros_like(wOutput);

    bOutput = 0.0;
    mbOutput = 0.0;
    rbOutput = 0.0;

    batchInput = xt::empty<double>({nMiniBatch, sizeX, sizeY, sizeZ});
    batchTarget = xt::empty<double>({nMiniBatch});
  }

  void Net::appendTrajectory(const Sample &sample) {
    ++trajectoryLength;
    if (data.size() == nData)
      data[writePos].first = assembleInput(sample);
    else
      data.emplace_back(assembleInput(sample), 0.0);
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

  xt::xtensor<int, 3> Net::assembleInput(const Sample &sample) const {
    xt::xtensor<int, 3> vInput(xt::empty<int>({sizeX, sizeY, sizeZ}));
    // Tile count
    for (int x{}; x < sizeX; ++x)
      for (int y{}; y < sizeY; ++y)
        for (int z{}; z < sizeZ; ++z)
          vInput(x, y, z) = tileMap.at(sample.state(x, y, z));
    return vInput;
  }

  double Net::infer(const Sample &sample) {
    forward(xt::view(assembleInput(sample), xt::newaxis(), xt::all(), xt::all(), xt::all()));
    return vOutput(0);
  }

  void Net::forward(const xt::xtensor<int, 4> &vInput) {
    if (vInput.shape(0) != nBatch) {
      nBatch = vInput.shape(0);
      vEmbeddings = xt::empty<double>({nBatch, sizeX, sizeY, sizeZ, nChannels});
      vConvsPre = xt::empty<double>({nBatch, nConvs, sizeX, sizeY, sizeZ, nChannels});
      vConvsPost = xt::empty_like(vConvsPre);
      vLocalPre = xt::empty<double>({nBatch, sizeX, sizeY, sizeZ});
      vLocalPost = xt::empty_like(vLocalPre);
      vGlobalPre = xt::empty<double>({nBatch, nFeatures});
      vGlobalPost = xt::empty_like(vGlobalPre);
      vOutput = xt::empty<double>({nBatch});
    }

    for (int i{}; i < nBatch; ++i)
      for (int x{}; x < sizeX; ++x)
        for (int y{}; y < sizeY; ++y)
          for (int z{}; z < sizeZ; ++z)
            xt::view(vEmbeddings, i, x, y, z, xt::all()) = wEmbeddings(vInput(i, x, y, z));

    for (int i{}; i < nConvs; ++i) {
      for (int x{}; x < sizeX; ++x) {
        for (int y{}; y < sizeY; ++y) {
          for (int z{}; z < sizeZ; ++z) {
            auto pre(xt::view(vConvsPre, xt::all(), i, x, y, z, xt::all()));
            pre = bConvs(i);
            for (int u{}; u < 3; ++u) {
              for (int v{}; v < 3; ++v) {
                for (int w{}; w < 3; ++w) {
                  int xSrc(x - 1 + u), ySrc(y - 1 + v), zSrc(z - 1 + w);
                  if (vInput.in_bounds(0, xSrc, ySrc, zSrc)) {
                    if (i) {
                      auto in(xt::view(vConvsPost, xt::all(), i - 1, xSrc, ySrc, zSrc, xt::newaxis(), xt::all()));
                      pre += xt::sum(xt::view(wConvs, i, u, v, w, xt::all(), xt::all()) * in, -1);
                    } else {
                      auto in(xt::view(vEmbeddings, xt::all(), xSrc, ySrc, zSrc, xt::newaxis(), xt::all()));
                      pre += xt::sum(xt::view(wConvs, i, u, v, w, xt::all(), xt::all()) * in, -1);
                    }
                  }
                }
              }
            }
            auto post(xt::view(vConvsPost, xt::all(), i, x, y, z, xt::all()));
            post = pre * leak + xt::clip(pre, -1.0, 1.0);
          }
        }
      }
    }

    vLocalPre = xt::sum(wLocal * xt::view(vConvsPost, xt::all(), nConvs - 1, xt::all(), xt::all(), xt::all(), xt::all()), -1) + bLocal;
    vLocalPost = vLocalPre * leak + xt::clip(vLocalPre, -1.0, 1.0);
    vGlobalPre = xt::sum(wGlobal * xt::view(vLocalPost, xt::all(), xt::newaxis(), xt::all(), xt::all(), xt::all()), {2, 3, 4}) + bGlobal;
    vGlobalPost = vGlobalPre * leak + xt::clip(vGlobalPre, -1.0, 1.0);
    vOutput = xt::sum(wOutput * vGlobalPost, -1) + bOutput;
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
    forward(batchInput);
    xt::xtensor<double, 1> vErrors(vOutput - batchTarget);
    double loss(xt::mean(xt::square(vErrors))());

    // Backprop
    xt::xtensor<double, 1> gvOutput(vErrors * 2 / nMiniBatch);
    double gbOutput(xt::sum(gvOutput)());
    xt::xtensor<double, 1> gwOutput(xt::sum(xt::view(gvOutput, xt::all(), xt::newaxis()) * vGlobalPost, 0));
    xt::xtensor<double, 2> gvGlobalPost(xt::empty_like(vGlobalPost));
    for (int i{}; i < nMiniBatch; ++i)
      for (int j{}; j < nFeatures; ++j)
        gvGlobalPost(i, j) = gvOutput(i) * wOutput(j);
    xt::xtensor<double, 2> gvGlobalPre(gvGlobalPost * (leak + (xt::abs(vGlobalPre) < 1.0)));
    xt::xtensor<double, 1> gbGlobal(xt::sum(gvGlobalPre, 0));
    xt::xtensor<double, 4> gwGlobal(xt::empty_like(wGlobal));
    for (int i{}; i < nFeatures; ++i)
      for (int x{}; x < sizeX; ++x)
        for (int y{}; y < sizeY; ++y)
          for (int z{}; z < sizeZ; ++z)
            gwGlobal(i, x, y, z) = xt::sum(xt::view(gvGlobalPre, xt::all(), i) * xt::view(vLocalPost, xt::all(), x, y, z))();
    xt::xtensor<double, 4> gvLocalPost(xt::sum(
      xt::view(gvGlobalPre, xt::all(), xt::all(), xt::newaxis(), xt::newaxis(), xt::newaxis()) * wGlobal, 1));
    xt::xtensor<double, 4> gvLocalPre(gvLocalPost * (leak + (xt::abs(vLocalPre) < 1.0)));
    double gbLocal(xt::sum(gvLocalPre)());
    xt::xtensor<double, 1> gwLocal(xt::sum(
      xt::view(gvLocalPre, xt::all(), xt::all(), xt::all(), xt::all(), xt::newaxis()) *
      xt::view(vConvsPost, xt::all(), nConvs - 1, xt::all(), xt::all(), xt::all(), xt::all()),
      {0, 1, 2, 3}));
    xt::xtensor<double, 6> gvConvsPost(xt::zeros_like(vConvsPost));
    for (int i{}; i < nMiniBatch; ++i)
      for (int x{}; x < sizeX; ++x)
        for (int y{}; y < sizeY; ++y)
          for (int z{}; z < sizeZ; ++z)
            for (int j{}; j < nChannels; ++j)
              gvConvsPost(i, nConvs - 1, x, y, z, j) = gvLocalPre(i, x, y, z) * wLocal(j);
    xt::xtensor<double, 6> gvConvsPre(xt::empty_like(vConvsPre));
    xt::xtensor<double, 2> gbConvs(xt::empty_like(bConvs));
    xt::xtensor<double, 6> gwConvs(xt::zeros_like(wConvs));
    xt::xtensor<double, 5> gvEmbeddings(xt::zeros_like(vEmbeddings));
    for (int i(nConvs - 1); i >= 0; --i) {
      auto gvConvsPreI(xt::view(gvConvsPre, xt::all(), i, xt::all(), xt::all(), xt::all(), xt::all()));
      gvConvsPreI = xt::view(gvConvsPost, xt::all(), i, xt::all(), xt::all(), xt::all(), xt::all()) *
        (leak + (xt::abs(xt::view(vConvsPre, xt::all(), i, xt::all(), xt::all(), xt::all(), xt::all())) < 1.0));
      xt::view(bConvs, i, xt::all()) = xt::sum(gvConvsPreI, {0, 1, 2, 3});
      for (int x{}; x < sizeX; ++x) {
        for (int y{}; y < sizeY; ++y) {
          for (int z{}; z < sizeZ; ++z) {
            for (int u{}; u < 3; ++u) {
              for (int v{}; v < 3; ++v) {
                for (int w{}; w < 3; ++w) {
                  int xSrc(x - 1 + u), ySrc(y - 1 + v), zSrc(z - 1 + w);
                  if (batchInput.in_bounds(0, xSrc, ySrc, zSrc)) {
                    for (int j{}; j < nChannels; ++j) {
                      for (int k{}; k < nChannels; ++k) {
                        if (i) {
                          gwConvs(i, u, v, w, j, k) += xt::sum(
                            xt::view(gvConvsPre, xt::all(), i, x, y, z, j) *
                            xt::view(vConvsPost, xt::all(), i - 1, xSrc, ySrc, zSrc, k))();
                          xt::view(gvConvsPost, xt::all(), i - 1, xSrc, ySrc, zSrc, k) +=
                            xt::view(gvConvsPre, xt::all(), i, x, y, z, j) * wConvs(i, u, v, w, j, k);
                        } else {
                          gwConvs(i, u, v, w, j, k) += xt::sum(
                            xt::view(gvConvsPre, xt::all(), i, x, y, z, j) *
                            xt::view(vEmbeddings, xt::all(), xSrc, ySrc, zSrc, k))();
                          xt::view(gvEmbeddings, xt::all(), x, y, z, k) +=
                            xt::view(gvConvsPre, xt::all(), i, x, y, z, j) * wConvs(i, u, v, w, j, k);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    xt::xtensor<double, 2> gwEmbeddings(xt::zeros_like(wEmbeddings));
    for (int i{}; i < nMiniBatch; ++i)
      for (int x{}; x < sizeX; ++x)
        for (int y{}; y < sizeY; ++y)
          for (int z{}; z < sizeZ; ++z)
            xt::view(gwEmbeddings, batchInput(i, x, y, z), xt::all()) += xt::view(gvEmbeddings, i, x, y, z, xt::all());

    // Adam
    mCorrector *= mRate;
    mwEmbeddings = mRate * mwEmbeddings + (1 - mRate) * gwEmbeddings;
    mwConvs = mRate * mwConvs + (1 - mRate) * gwConvs;
    mbConvs = mRate * mbConvs + (1 - mRate) * gbConvs;
    mwLocal = mRate * mwLocal + (1 - mRate) * gwLocal;
    mbLocal = mRate * mbLocal + (1 - mRate) * gbLocal;
    mwGlobal = mRate * mwGlobal + (1 - mRate) * gwGlobal;
    mbGlobal = mRate * mbGlobal + (1 - mRate) * gbGlobal;
    mwOutput = mRate * mwOutput + (1 - mRate) * gwOutput;
    mbOutput = mRate * mbOutput + (1 - mRate) * gbOutput;

    rCorrector *= rRate;
    rwEmbeddings = rRate * rwEmbeddings + (1 - rRate) * xt::square(gwEmbeddings);
    rwConvs = rRate * rwConvs + (1 - rRate) * xt::square(gwConvs);
    rbConvs = rRate * rbConvs + (1 - rRate) * xt::square(gbConvs);
    rwLocal = rRate * rwLocal + (1 - rRate) * xt::square(gwLocal);
    rbLocal = rRate * rbLocal + (1 - rRate) * (gbLocal * gbLocal);
    rwGlobal = rRate * rwGlobal + (1 - rRate) * xt::square(gwGlobal);
    rbGlobal = rRate * rbGlobal + (1 - rRate) * xt::square(gbGlobal);
    rwOutput = rRate * rwOutput + (1 - rRate) * xt::square(gwOutput);
    rbOutput = rRate * rbOutput + (1 - rRate) * (gbOutput * gbOutput);

    wEmbeddings -= lRate * mwEmbeddings / ((1 - mCorrector) * (xt::sqrt(rwEmbeddings / (1 - rCorrector)) + 1e-8));
    wConvs -= lRate * mwConvs / ((1 - mCorrector) * (xt::sqrt(rwConvs / (1 - rCorrector)) + 1e-8));
    bConvs -= lRate * mbConvs / ((1 - mCorrector) * (xt::sqrt(rbConvs / (1 - rCorrector)) + 1e-8));
    wLocal -= lRate * mwLocal / ((1 - mCorrector) * (xt::sqrt(rwLocal / (1 - rCorrector)) + 1e-8));
    bLocal -= lRate * mbLocal / ((1 - mCorrector) * (std::sqrt(rbLocal / (1 - rCorrector)) + 1e-8));
    wGlobal -= lRate * mwGlobal / ((1 - mCorrector) * (xt::sqrt(rwGlobal / (1 - rCorrector)) + 1e-8));
    bGlobal -= lRate * mbGlobal / ((1 - mCorrector) * (xt::sqrt(rbGlobal / (1 - rCorrector)) + 1e-8));
    wOutput -= lRate * mwOutput / ((1 - mCorrector) * (xt::sqrt(rwOutput / (1 - rCorrector)) + 1e-8));
    bOutput -= lRate * mbOutput / ((1 - mCorrector) * (std::sqrt(rbOutput / (1 - rCorrector)) + 1e-8));

    return loss;
  }
}
