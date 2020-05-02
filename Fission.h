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

  enum {
    GoalAvgPower,
    GoalEfficiency,
    GoalAvgBreed,
    GoalHeatNeutral,
    GoalNoOverCooling,
    GoalNoInvalid
  };

  struct Settings {
    int sizeX, sizeY, sizeZ;
    double fuelBasePower, fuelBaseHeat;
    int limit[Air];
    double coolingRates[Cell];
    bool ensureActiveCoolerAccessible;
    std::vector<int> goals;

    void bestPower(bool preferHeatNeutral);
    void bestEfficiency(bool preferHeatNeutral);
    void bestBreeder(bool preferHeatNeutral);
  };

  struct Evaluation {
    // Raw
    double breed, powerMult, heatMult, cooling;
    int nInvalid;
    // Computed
    double heat, netHeat, dutyCycle, power, avgPower, avgBreed, efficiency;

    void compute(const Settings &settings);
    bool asGoodAs(int goal, const Evaluation &o) const;
    bool asGoodAs(const std::vector<int> &goals, const Evaluation &o) const;
    bool betterThan(int goal, const Evaluation &o) const;
    bool betterThan(const std::vector<int> &goals, const Evaluation &o) const;
  };

  Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state);
}

#endif
