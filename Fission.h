#ifndef _FISSION_H_
#define _FISSION_H_
#include <xtensor/xtensor.hpp>
#include <string>

namespace Tile {
  enum {
    // Cooler
    Water, Redstone, Quartz, Gold, Glowstone,
    Lapis, Diamond, Helium, Enderium, Cryotheum,
    Iron, Emerald, Copper, Tin, Magnesium,
    // Other
    Air, Cell, Moderator, Casing
  };
  std::string toString(int tile);
}

std::string stateToString(const xt::xtensor<int, 3> &state);

struct Settings {
  int sizeX, sizeY, sizeZ;
  double fuelBasePower, fuelBaseHeat;
  bool allowedCoolers[Tile::Air];
  double coolingRate[Tile::Air];
};

struct Evaluation {
  bool valid;
  double power, heat, cooling;
  double netHeat() const { return heat - cooling; }
  double effPower() const { return valid ? power * std::min(1.0, cooling / heat) : -1.0; }
};

Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state);

#endif
