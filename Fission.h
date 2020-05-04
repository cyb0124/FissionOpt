#ifndef _FISSION_H_
#define _FISSION_H_
#include <xtensor/xtensor.hpp>
#include <string>

namespace Fission {
  constexpr int neutronReach(4);
  constexpr double modPower(1.0), modHeat(2.0);

  enum {
    // Cooler
    Water, Redstone, Quartz, Gold, Glowstone,
    Lapis, Diamond, Helium, Enderium, Cryotheum,
    Iron, Emerald, Copper, Tin, Magnesium, Active,
    // Other
    Cell = Active * 2, Moderator, Air
  };

  struct Settings {
    int sizeX, sizeY, sizeZ;
    double fuelBasePower, fuelBaseHeat;
    int limit[Air];
    double coolingRates[Cell];
    bool ensureActiveCoolerAccessible;
    bool ensureHeatNeutral;
    bool breeder;
  };

  struct Evaluation {
    // Raw
    double powerMult, heatMult, cooling;
    int breed;
    // Computed
    double heat, netHeat, dutyCycle, power, avgPower, avgBreed;

    void compute(const Settings &settings);
  };

  Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state, std::vector<std::tuple<int, int, int>> *invalidTiles);
}

#endif
