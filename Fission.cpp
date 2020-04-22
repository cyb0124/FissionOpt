#include <xtensor/xview.hpp>
#include "Fission.h"

static int getTileWithCasing(const xt::xtensor<int, 3> &state, int x, int y, int z) {
  if (state.in_bounds(x, y, z))
    return state(x, y, z);
  return Tile::Casing;
}

static int getEffWithCasing(const xt::xtensor<int, 3> &effs, int x, int y, int z) {
  if (effs.in_bounds(x, y, z))
    return effs(x, y, z);
  return 0;
}

static bool hasConnected(const xt::xtensor<int, 3> &state, int x, int y, int z, int dx, int dy, int dz) {
  for (int n{}; n < 5; ++n) {
    x += dx; y += dy; z += dz;
    int tile(getTileWithCasing(state, x, y, z));
    if (tile == Tile::Cell)
      return true;
    if (tile != Tile::Moderator)
      return false;
  }
  return false;
}

static int countEff(const xt::xtensor<int, 3> &state, int x, int y, int z) {
  if (state(x, y, z) != Tile::Cell)
    return 0;
  return 1
    + hasConnected(state, x, y, z, -1, 0, 0)
    + hasConnected(state, x, y, z, +1, 0, 0)
    + hasConnected(state, x, y, z, 0, -1, 0)
    + hasConnected(state, x, y, z, 0, +1, 0)
    + hasConnected(state, x, y, z, 0, 0, -1)
    + hasConnected(state, x, y, z, 0, 0, +1);
}

static int countNeighbor(const int neighbors[6], int tile) {
  int result{};
  for (int i{}; i < 6; ++i)
    if (neighbors[i] == tile)
      ++result;
  return result;
}

Evaluation evaluate(const Settings &settings, const xt::xtensor<int, 3> &state) {
  Evaluation result{true};
  xt::xtensor<int, 3> effs(xt::empty<int>(state.shape()));
  for (int x{}; x < state.shape(0); ++x) {
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        int eff(countEff(state, x, y, z));
        effs(x, y, z) = eff;
        result.power += eff;
        result.heat += eff * (eff + 1) / 2.0;
      }
    }
  }

  for (int x{}; x < state.shape(0); ++x) {
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        int tile(state(x, y, z));
        if (tile == Tile::Moderator) {
          int eff(
            + getEffWithCasing(effs, x - 1, y, z)
            + getEffWithCasing(effs, x + 1, y, z)
            + getEffWithCasing(effs, x, y - 1, z)
            + getEffWithCasing(effs, x, y + 1, z)
            + getEffWithCasing(effs, x, y, z - 1)
            + getEffWithCasing(effs, x, y, z + 1));
          if (!eff)
            return {};
          result.power += eff / 6.0;
          result.heat += eff / 3.0;
        } else if (tile < Tile::Air) {
          int neighbors[] {
            getTileWithCasing(state, x - 1, y, z),
            getTileWithCasing(state, x + 1, y, z),
            getTileWithCasing(state, x, y - 1, z),
            getTileWithCasing(state, x, y + 1, z),
            getTileWithCasing(state, x, y, z - 1),
            getTileWithCasing(state, x, y, z + 1)
          };
          switch (tile) {
            case Tile::Water:
              if (!countNeighbor(neighbors, Tile::Cell) && !countNeighbor(neighbors, Tile::Moderator))
                return {};
              break;
            case Tile::Redstone:
              if (!countNeighbor(neighbors, Tile::Cell))
                return {};
              break;
            case Tile::Quartz:
              if (!countNeighbor(neighbors, Tile::Moderator))
                return {};
              break;
            case Tile::Gold:
              if (!countNeighbor(neighbors, Tile::Water) || !countNeighbor(neighbors, Tile::Redstone))
                return {};
              break;
            case Tile::Glowstone:
              if (countNeighbor(neighbors, Tile::Moderator) < 2)
                return {};
              break;
            case Tile::Lapis:
              if (!countNeighbor(neighbors, Tile::Cell) || !countNeighbor(neighbors, Tile::Casing))
                return {};
              break;
            case Tile::Diamond:
              if (!countNeighbor(neighbors, Tile::Water) || !countNeighbor(neighbors, Tile::Quartz))
                return {};
              break;
            case Tile::Helium:
              if (countNeighbor(neighbors, Tile::Redstone) != 1 || !countNeighbor(neighbors, Tile::Casing))
                return {};
              break;
            case Tile::Enderium:
              if (countNeighbor(neighbors, Tile::Casing) != 3
                || x && x != state.shape(0) - 1
                || y && y != state.shape(1) - 1
                || z && z != state.shape(2) - 1)
                return {};
              break;
            case Tile::Cryotheum:
              if (countNeighbor(neighbors, Tile::Cell) < 2)
                return {};
              break;
            case Tile::Iron:
              if (!countNeighbor(neighbors, Tile::Gold))
                return {};
              break;
            case Tile::Emerald:
              if (!countNeighbor(neighbors, Tile::Moderator) || !countNeighbor(neighbors, Tile::Cell))
                return {};
              break;
            case Tile::Copper:
              if (!countNeighbor(neighbors, Tile::Glowstone))
                return {};
              break;
            case Tile::Tin:
              for (int i{}; i < 6; i += 2)
                if (neighbors[i] == Tile::Lapis && neighbors[i + 1] == Tile::Lapis)
                  goto valid;
              return {};
            case Tile::Magnesium:
              if (!countNeighbor(neighbors, Tile::Casing) || !countNeighbor(neighbors, Tile::Moderator))
                return {};
          }
          valid: result.cooling += settings.coolingRate[tile];
        }
      }
    }
  }
  result.power *= settings.fuelBasePower;
  result.heat *= settings.fuelBaseHeat;
  return result;
}
