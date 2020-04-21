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
    Air, Cell, Moderator, Casing, Free
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

double evaluate(const Settings &settings, const xt::xtensor<int, 3> &state);

#endif
