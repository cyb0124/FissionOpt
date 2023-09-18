#ifndef _FISSION_H_
#define _FISSION_H_
#include <xtensor/xtensor.hpp>
#include <string>

namespace Fission {
  using Coords = std::vector<std::tuple<int, int, int>>;

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

  enum {
    GoalPower,
    GoalBreeder,
    GoalEfficiency
  };

  struct Settings {
    int sizeX, sizeY, sizeZ;
    double fuelBasePower, fuelBaseHeat;
    int limit[Air];
    double coolingRates[Cell];
    bool ensureActiveCoolerAccessible;
    bool ensureHeatNeutral;
    int goal;
    bool symX, symY, symZ;
  };

  struct Evaluation {
    // Raw
    Coords invalidTiles;
    double powerMult, heatMult, cooling;
    int breed;
    // Computed
    double heat, netHeat, dutyCycle, avgMult, power, avgPower, avgBreed, efficiency;

    void compute(const Settings &settings);
  };

  class Evaluator {
    const Settings &settings;
    xt::xtensor<int, 3> mults, rules;
    xt::xtensor<bool, 3> isActive, isModeratorInLine, visited, accessible;
    const xt::xtensor<int, 3> *state;
    int compatibleTile;

    int getTileSafe(int x, int y, int z) const;
    int getMultSafe(int x, int y, int z) const;
    bool countMult(int x, int y, int z, int dx, int dy, int dz);
    int countMult(int x, int y, int z);
    bool isActiveSafe(int tile, int x, int y, int z) const;
    int countActiveNeighbors(int tile, int x, int y, int z) const;
    bool isTileSafe(int tile, int x, int y, int z) const;
    int countNeighbors(int tile, int x, int y, int z) const;
    int countCasingNeighbors(int x, int y, int z) const;
    bool checkAccessibility(int compatibleTile, int x, int y, int z);
    bool checkCompatible(int compatibleTile, int x, int y, int z);
    bool checkAccessibility(int x, int y, int z);
  public:
    Evaluator(const Settings &settings);
    void run(const xt::xtensor<int, 3> &state, Evaluation &result);
  };
}

#endif
