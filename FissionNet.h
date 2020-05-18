#ifndef _FISSION_NET_H_
#define _FISSION_NET_H_
#include <unordered_map>
#include "OptFission.h"

namespace Fission {
  constexpr int nChannels(4), nConvs(2), nFeatures(16);
  constexpr int nMiniBatch(64), nEpoch(1), nData(1'000'000);
  constexpr double lRate(0.01), mRate(0.9), rRate(0.999), leak(0.1);

  class Net {
    Opt &opt;
    int sizeX, sizeY, sizeZ;
    std::unordered_map<int, int> tileMap;

    // Data Pool
    xt::xtensor<int, 4> batchInput;
    xt::xtensor<double, 1> batchTarget;
    std::vector<std::pair<xt::xtensor<int, 3>, double>> data;
    int trajectoryLength, writePos;

    // Model
    int nLastBatch;
    double mCorrector, rCorrector;
    // nCategories * nChannels
    xt::xtensor<double, 2> wEmbeddings, mwEmbeddings, rwEmbeddings;
    // nConvs * 3 * 3 * 3 * nChannels * nChannels
    xt::xtensor<double, 6> wConvs, mwConvs, rwConvs;
    // nConvs * nChannels
    xt::xtensor<double, 2> bConvs, mbConvs, rbConvs;
    // nChannels
    xt::xtensor<double, 1> wLocal, mwLocal, rwLocal;
    double bLocal, mbLocal, rbLocal;
    // nFeatures * sizeX * sizeY * sizeZ
    xt::xtensor<double, 4> wGlobal, mwGlobal, rwGlobal;
    // nFeatures
    xt::xtensor<double, 1> bGlobal, mbGlobal, rbGlobal;
    // nFeatures
    xt::xtensor<double, 1> wOutput, mwOutput, rwOutput;
    double bOutput, mbOutput, rbOutput;

    // Intermediates
    // nBatch * sizeX * sizeY * sizeZ * nChannels
    xt::xtensor<double, 5> vEmbeddings;
    // nBatch * nConvs * sizeX * sizeY * sizeZ * nChannels
    xt::xtensor<double, 6> vConvsPre, vConvsPost;
    // nBatch * sizeX * sizeY * sizeZ
    xt::xtensor<double, 4> vLocalPre, vLocalPost;
    // nBatch * nFeatures
    xt::xtensor<double, 2> vGlobalPre, vGlobalPost;
    // nBatch
    xt::xtensor<double, 1> vOutput;

    void forward(const xt::xtensor<int, 4> &vInput);
    xt::xtensor<int, 3> assembleInput(const Sample &sample) const;
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
