#ifndef _FISSION_H_
#define _FISSION_H_
#include <xtensor/xtensor.hpp>
#include <string>

constexpr int neutronReach(4);
constexpr double modPower(1.0), modHeat(2.0);

namespace Tile {
  enum {
    // Cooler
    Water, Redstone, Quartz, Gold, Glowstone,
    Lapis, Diamond, Helium, Enderium, Cryotheum,
    Iron, Emerald, Copper, Tin, Magnesium, Active,
    // Other
    Air = Active * 2, Cell, Moderator, Casing
  };
}

struct Settings {
  int sizeX, sizeY, sizeZ;
  double fuelBasePower, fuelBaseHeat;
  int coolerLimits[Tile::Air];
  double coolingRates[Tile::Air];
  bool ensureActiveCoolerAccessible;
};

struct Evaluation {
  bool valid;
  double power, heat, cooling;
  double netHeat() const { return heat - cooling; }
  double dutyCycle() const { return std::min(1.0, cooling / heat); }
  double effPower() const { return valid ? power * dutyCycle() : -1.0; }
};

Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state);

#endif
