#include <xtensor/xview.hpp>
#include "Fission.h"

namespace Fission {
  void Settings::bestPower(bool preferHeatNeutral) {
    if (preferHeatNeutral) {
      goals = {
        GoalValid,
        GoalHeatNeutral,
        GoalAvgPower,
        GoalNoOverCooling
      };
    } else {
      goals = {
        GoalValid,
        GoalAvgPower,
        GoalHeatNeutral,
        GoalNoOverCooling
      };
    }
  }

  void Settings::bestEfficiency(bool preferHeatNeutral) {
    if (preferHeatNeutral) {
      goals = {
        GoalValid,
        GoalHeatNeutral,
        GoalEfficiency,
        GoalAvgPower,
        GoalNoOverCooling
      };
    } else {
      goals = {
        GoalValid,
        GoalEfficiency,
        GoalAvgPower,
        GoalHeatNeutral,
        GoalNoOverCooling
      };
    }
  }

  void Settings::bestBreeder(bool preferHeatNeutral) {
    if (preferHeatNeutral) {
      goals = {
        GoalValid,
        GoalHeatNeutral,
        GoalAvgBreed,
        GoalAvgPower,
        GoalNoOverCooling
      };
    } else {
      goals = {
        GoalValid,
        GoalAvgBreed,
        GoalAvgPower,
        GoalHeatNeutral,
        GoalNoOverCooling
      };
    }
  }

  void Evaluation::compute(const Settings &settings) {
    heat = settings.fuelBaseHeat * heatMult;
    netHeat = heat - cooling;
    dutyCycle = std::min(1.0, cooling / heat);
    power = settings.fuelBasePower * powerMult;
    avgPower = power * dutyCycle;
    avgBreed = breed * dutyCycle;
    efficiency = std::max(1.0, powerMult / breed);
  }

  bool Evaluation::asGoodAs(int goal, const Evaluation &o) const {
    switch (goal) {
      default: // GoalValid
        return valid >= o.valid;
      case GoalAvgPower:
        return avgPower + 0.01 >= o.avgPower;
      case GoalEfficiency:
        return efficiency + 0.01 >= o.efficiency;
      case GoalAvgBreed:
        return avgBreed + 0.01 >= o.avgBreed;
      case GoalHeatNeutral:
        if (netHeat <= 0)
          return true;
        return netHeat - 0.01 <= o.netHeat;
      case GoalNoOverCooling:
        return cooling - 0.01 <= o.cooling;
    }
  }

  bool Evaluation::betterThan(int goal, const Evaluation &o) const {
    switch (goal) {
      default: // GoalValid
        return valid > o.valid;
      case GoalAvgPower:
        return avgPower > o.avgPower + 0.01;
      case GoalEfficiency:
        return efficiency > o.efficiency + 0.01;
      case GoalAvgBreed:
        return avgBreed > o.avgBreed + 0.01;
      case GoalHeatNeutral:
        if (o.netHeat <= 0)
          return false;
        return netHeat < o.netHeat - 0.01;
      case GoalNoOverCooling:
        return cooling < o.cooling - 0.01;
    }
  }

  bool Evaluation::asGoodAs(const std::vector<int> &goals, const Evaluation &o) const {
    for (int goal : goals)
      if (betterThan(goal, o))
        return true;
      else if (!asGoodAs(goal, o))
        return false;
    return true;
  }

  bool Evaluation::betterThan(const std::vector<int> &goals, const Evaluation &o) const {
    for (int goal : goals)
      if (betterThan(goal, o))
        return true;
      else if (!asGoodAs(goal, o))
        return false;
    return false;
  }

  namespace {
    int getTileWithCasing(const xt::xtensor<int, 3> &state, int x, int y, int z) {
      if (!state.in_bounds(x, y, z))
        return Casing;
      return state(x, y, z);
    }
    
    int getTileWithCasingWithoutInactiveModerator(const xt::xtensor<int, 3> &state, const xt::xtensor<bool, 3> &isModeratorActive, int x, int y, int z) {
      if (!state.in_bounds(x, y, z))
        return Casing;
      int tile(state(x, y, z));
      if (tile == Moderator && !isModeratorActive(x, y, z))
        return -1;
      return tile;
    }

    int getMultWithCasing(const xt::xtensor<int, 3> &mults, int x, int y, int z) {
      if (!mults.in_bounds(x, y, z))
        return 0;
      return mults(x, y, z);
    }

    bool countMult(const xt::xtensor<int, 3> &state, xt::xtensor<bool, 3> &isModeratorInLine, int x, int y, int z, int dx, int dy, int dz) {
      for (int n{}; n <= neutronReach; ++n) {
        x += dx; y += dy; z += dz;
        int tile(getTileWithCasing(state, x, y, z));
        if (tile == Cell) {
          for (int i{}; i < n; ++i) {
            x -= dx; y -= dy; z -= dz;
            isModeratorInLine(x, y, z) = true;
          }
          return true;
        } else if (tile != Moderator) {
          return false;
        }
      }
      return false;
    }

    int countMult(const xt::xtensor<int, 3> &state, xt::xtensor<bool, 3> &isModeratorInLine, int x, int y, int z) {
      if (state(x, y, z) != Cell)
        return 0;
      return 1
        + countMult(state, isModeratorInLine, x, y, z, -1, 0, 0)
        + countMult(state, isModeratorInLine, x, y, z, +1, 0, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, -1, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, +1, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, 0, -1)
        + countMult(state, isModeratorInLine, x, y, z, 0, 0, +1);
    }

    int countNeighbor(const int neighbors[6], int tile) {
      int result{};
      for (int i{}; i < 6; ++i)
        if (neighbors[i] == tile)
          ++result;
      return result;
    }

    class AccessibilityChecker {
      const xt::xtensor<int, 3> &state;
      int compatibleTile;
      xt::xtensor<bool, 3> visited;
    public:
      AccessibilityChecker(const xt::xtensor<int, 3> &state, int compatibleTile)
        :state(state), compatibleTile(compatibleTile),
        visited(xt::zeros<bool>(state.shape())) {}

      bool run(int x, int y, int z) {
        if (!state.in_bounds(x, y, z))
          return true;
        if (visited(x, y, z))
          return false;
        visited(x, y, z) = true;
        int tile(state(x, y, z));
        if (tile != Air && tile != compatibleTile)
          return false;
        return
          run(x - 1, y, z) ||
          run(x + 1, y, z) ||
          run(x, y - 1, z) ||
          run(x, y + 1, z) ||
          run(x, y, z - 1) ||
          run(x, y, z + 1);
      }
    };
  }

  Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state) {
    Evaluation result{true};
    xt::xtensor<int, 3> mults(xt::empty<int>(state.shape()));
    xt::xtensor<bool, 3> isModeratorInLine(xt::zeros<bool>(state.shape()));
    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          int mult(countMult(state, isModeratorInLine, x, y, z));
          mults(x, y, z) = mult;
          if (mult) {
            ++result.breed;
            result.powerMult += mult;
            result.heatMult += mult * (mult + 1) / 2.0;
          }
        }
      }
    }

    xt::xtensor<bool, 3> isModeratorActive(xt::zeros<bool>(state.shape()));
    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          if (state(x, y, z) == Moderator) {
            int mult(
              + getMultWithCasing(mults, x - 1, y, z)
              + getMultWithCasing(mults, x + 1, y, z)
              + getMultWithCasing(mults, x, y - 1, z)
              + getMultWithCasing(mults, x, y + 1, z)
              + getMultWithCasing(mults, x, y, z - 1)
              + getMultWithCasing(mults, x, y, z + 1));
            isModeratorActive(x, y, z) = mult;
            if (!mult && !isModeratorInLine(x, y, z))
              return {};
            result.powerMult += mult * (modPower / 6.0);
            result.heatMult += mult * (modHeat / 6.0);
          }
        }
      }
    }

    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          int tile(state(x, y, z));
          if (tile < Cell) {
            int rule;
            if (tile < Active) {
              rule = tile;
            } else {
              if (settings.ensureActiveCoolerAccessible && !AccessibilityChecker(state, tile).run(x, y, z))
                return {};
              rule = tile - Active;
            }
            int neighbors[] {
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x - 1, y, z),
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x + 1, y, z),
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x, y - 1, z),
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x, y + 1, z),
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x, y, z - 1),
              getTileWithCasingWithoutInactiveModerator(state, isModeratorActive, x, y, z + 1)
            };
            switch (rule) {
              case Water:
                if (!countNeighbor(neighbors, Cell) && !countNeighbor(neighbors, Moderator))
                  return {};
                break;
              case Redstone:
                if (!countNeighbor(neighbors, Cell))
                  return {};
                break;
              case Quartz:
                if (!countNeighbor(neighbors, Moderator))
                  return {};
                break;
              case Gold:
                if (!countNeighbor(neighbors, Water) || !countNeighbor(neighbors, Redstone))
                  return {};
                break;
              case Glowstone:
                if (countNeighbor(neighbors, Moderator) < 2)
                  return {};
                break;
              case Lapis:
                if (!countNeighbor(neighbors, Cell) || !countNeighbor(neighbors, Casing))
                  return {};
                break;
              case Diamond:
                if (!countNeighbor(neighbors, Water) || !countNeighbor(neighbors, Quartz))
                  return {};
                break;
              case Helium:
                if (countNeighbor(neighbors, Redstone) != 1 || !countNeighbor(neighbors, Casing))
                  return {};
                break;
              case Enderium:
                if (countNeighbor(neighbors, Casing) != 3
                  || x && x != state.shape(0) - 1
                  || y && y != state.shape(1) - 1
                  || z && z != state.shape(2) - 1)
                  return {};
                break;
              case Cryotheum:
                if (countNeighbor(neighbors, Cell) < 2)
                  return {};
                break;
              case Iron:
                if (!countNeighbor(neighbors, Gold))
                  return {};
                break;
              case Emerald:
                if (!countNeighbor(neighbors, Moderator) || !countNeighbor(neighbors, Cell))
                  return {};
                break;
              case Copper:
                if (!countNeighbor(neighbors, Glowstone))
                  return {};
                break;
              case Tin:
                for (int i{}; i < 6; i += 2)
                  if (neighbors[i] == Lapis && neighbors[i + 1] == Lapis)
                    goto valid;
                return {};
              case Magnesium:
                if (!countNeighbor(neighbors, Casing) || !countNeighbor(neighbors, Moderator))
                  return {};
            }
            valid: result.cooling += settings.coolingRates[tile];
          }
        }
      }
    }
    result.compute(settings);
    return result;
  }
}
