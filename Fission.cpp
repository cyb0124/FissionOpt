#include <xtensor/xview.hpp>
#include "Fission.h"

namespace Fission {
  void Evaluation::compute(const Settings &settings) {
    heat = settings.fuelBaseHeat * heatMult;
    netHeat = heat - cooling;
    dutyCycle = std::min(1.0, cooling / heat);
    power = settings.fuelBasePower * powerMult;
    avgPower = power * dutyCycle;
    avgBreed = breed * dutyCycle;
  }

  bool Evaluation::feasible(const Settings &settings) const {
    return !settings.ensureHeatNeutral || netHeat <= 0.0;
  }

  double Evaluation::fitness(const Settings &settings) const {
    return settings.breeder ? avgBreed : avgPower;
  }

  namespace {
    int getTileSafe(const xt::xtensor<int, 3> &state, int x, int y, int z) {
      if (!state.in_bounds(x, y, z))
        return -1;
      return state(x, y, z);
    }

    int getMultSafe(const xt::xtensor<int, 3> &mults, int x, int y, int z) {
      if (!mults.in_bounds(x, y, z))
        return 0;
      return mults(x, y, z);
    }

    bool countMult(const xt::xtensor<int, 3> &state, xt::xtensor<bool, 3> &isModeratorInLine, int x, int y, int z, int dx, int dy, int dz) {
      for (int n{}; n <= neutronReach; ++n) {
        x += dx; y += dy; z += dz;
        int tile(getTileSafe(state, x, y, z));
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
      return 1
        + countMult(state, isModeratorInLine, x, y, z, -1, 0, 0)
        + countMult(state, isModeratorInLine, x, y, z, +1, 0, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, -1, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, +1, 0)
        + countMult(state, isModeratorInLine, x, y, z, 0, 0, -1)
        + countMult(state, isModeratorInLine, x, y, z, 0, 0, +1);
    }

    bool isActiveSafe(const xt::xtensor<int, 3> &state, const xt::xtensor<bool, 3> &active, int tile, int x, int y, int z) {
      if (!state.in_bounds(x, y, z))
        return false;
      return state(x, y, z) == tile && active(x, y, z);
    }

    int countActiveNeighbors(const xt::xtensor<int, 3> &state, const xt::xtensor<bool, 3> &active, int tile, int x, int y, int z) {
      return
        + isActiveSafe(state, active, tile, x - 1, y, z)
        + isActiveSafe(state, active, tile, x + 1, y, z)
        + isActiveSafe(state, active, tile, x, y - 1, z)
        + isActiveSafe(state, active, tile, x, y + 1, z)
        + isActiveSafe(state, active, tile, x, y, z - 1)
        + isActiveSafe(state, active, tile, x, y, z + 1);
    }

    bool isTileSafe(const xt::xtensor<int, 3> &state, int tile, int x, int y, int z) {
      if (!state.in_bounds(x, y, z))
        return false;
      return state(x, y, z) == tile;
    }

    int countNeighbors(const xt::xtensor<int, 3> &state, int tile, int x, int y, int z) {
      return
        + isTileSafe(state, tile, x - 1, y, z)
        + isTileSafe(state, tile, x + 1, y, z)
        + isTileSafe(state, tile, x, y - 1, z)
        + isTileSafe(state, tile, x, y + 1, z)
        + isTileSafe(state, tile, x, y, z - 1)
        + isTileSafe(state, tile, x, y, z + 1);
    }

    int countCasingNeighbors(const xt::xtensor<int, 3> &state, int x, int y, int z) {
      return
        + !state.in_bounds(x - 1, y, z)
        + !state.in_bounds(x + 1, y, z)
        + !state.in_bounds(x, y - 1, z)
        + !state.in_bounds(x, y + 1, z)
        + !state.in_bounds(x, y, z - 1)
        + !state.in_bounds(x, y, z + 1);
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

  Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state, std::vector<std::tuple<int, int, int>> *invalidTiles) {
    Evaluation result{true};
    xt::xtensor<int, 3> mults(xt::empty<int>(state.shape()));
    xt::xtensor<int, 3> rules(xt::empty<int>(state.shape()));
    xt::xtensor<bool, 3> isActive(xt::zeros<bool>(state.shape()));
    xt::xtensor<bool, 3> isModeratorInLine(xt::zeros<bool>(state.shape()));
    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          int tile(state(x, y, z));
          if (tile == Cell) {
            int mult(countMult(state, isModeratorInLine, x, y, z));
            mults(x, y, z) = mult;
            rules(x, y, z) = -1;
            ++result.breed;
            result.powerMult += mult;
            result.heatMult += mult * (mult + 1) / 2.0;
          } else {
            mults(x, y, z) = 0;
            if (tile < Active)
              rules(x, y, z) = tile;
            else if (tile < Cell)
              if (settings.ensureActiveCoolerAccessible && !AccessibilityChecker(state, tile).run(x, y, z))
                rules(x, y, z) = -1;
              else
                rules(x, y, z) = tile - Active;
            else
              rules(x, y, z) = -1;
          }
        }
      }
    }

    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          if (state(x, y, z) == Moderator) {
            int mult(
              + getMultSafe(mults, x - 1, y, z)
              + getMultSafe(mults, x + 1, y, z)
              + getMultSafe(mults, x, y - 1, z)
              + getMultSafe(mults, x, y + 1, z)
              + getMultSafe(mults, x, y, z - 1)
              + getMultSafe(mults, x, y, z + 1));
            if (mult) {
              isActive(x, y, z) = true;
              result.powerMult += mult * (modPower / 6.0);
              result.heatMult += mult * (modHeat / 6.0);
            } else if (invalidTiles && !isModeratorInLine(x, y, z)) {
              invalidTiles->emplace_back(x, y, z);
            }
          } else switch (rules(x, y, z)) {
            case Redstone:
              isActive(x, y, z) = countNeighbors(state, Cell, x, y, z);
              break;
            case Lapis:
              isActive(x, y, z) = countNeighbors(state, Cell, x, y, z)
                && countCasingNeighbors(state, x, y, z);
              break;
            case Enderium:
              isActive(x, y, z) = countCasingNeighbors(state, x, y, z) == 3
                && (!x || x == state.shape(0) - 1)
                && (!y || y == state.shape(1) - 1)
                && (!z || z == state.shape(2) - 1);
              break;
            case Cryotheum:
              isActive(x, y, z) = countNeighbors(state, Cell, x, y, z) >= 2;
          }
        }
      }
    }
    
    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          switch (rules(x, y, z)) {
            case Water:
              isActive(x, y, z) = countNeighbors(state, Cell, x, y, z)
                || countActiveNeighbors(state, isActive, Moderator, x, y, z);
              break;
            case Quartz:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Moderator, x, y, z);
              break;
            case Glowstone:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Moderator, x, y, z) >= 2;
              break;
            case Helium:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Redstone, x, y, z) == 1
                && countCasingNeighbors(state, x, y, z);
              break;
            case Emerald:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Moderator, x, y, z)
                && countNeighbors(state, Cell, x, y, z);
              break;
            case Tin:
              isActive(x, y, z) =
                isActiveSafe(state, isActive, Lapis, x - 1, y, z) &&
                isActiveSafe(state, isActive, Lapis, x + 1, y, z) ||
                isActiveSafe(state, isActive, Lapis, x, y - 1, z) &&
                isActiveSafe(state, isActive, Lapis, x, y + 1, z) ||
                isActiveSafe(state, isActive, Lapis, x, y, z - 1) &&
                isActiveSafe(state, isActive, Lapis, x, y, z + 1);
              break;
            case Magnesium:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Moderator, x, y, z)
                && countCasingNeighbors(state, x, y, z);
          }
        }
      }
    }
    
    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          switch (rules(x, y, z)) {
            case Gold:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Water, x, y, z)
                && countActiveNeighbors(state, isActive, Redstone, x, y, z);
              break;
            case Diamond:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Water, x, y, z)
                && countActiveNeighbors(state, isActive, Quartz, x, y, z);
              break;
            case Copper:
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Glowstone, x, y, z);
          }
        }
      }
    }

    for (int x{}; x < state.shape(0); ++x) {
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          int tile(state(x, y, z));
          if (tile < Cell) {
            if (rules(x, y, z) == Iron)
              isActive(x, y, z) = countActiveNeighbors(state, isActive, Gold, x, y, z);
            if (isActive(x, y, z))
              result.cooling += settings.coolingRates[tile];
            else if (invalidTiles)
              invalidTiles->emplace_back(x, y, z);
          }
        }
      }
    }

    result.compute(settings);
    return result;
  }
}
