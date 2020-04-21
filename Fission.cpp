#include <xtensor/xview.hpp>
#include "Fission.h"

namespace Tile {
  std::string toString(int tile) {
    switch (tile) {
      case Water:
        return "Wt";
      case Redstone:
        return "Rs";
      case Quartz:
        return "Qz";
      case Gold:
        return "Au";
      case Glowstone:
        return "Gs";
      case Lapis:
        return "Lp";
      case Diamond:
        return "Dm";
      case Helium:
        return "He";
      case Enderium:
        return "Ed";
      case Cryotheum:
        return "Cr";
      case Iron:
        return "Fe";
      case Emerald:
        return "Em";
      case Copper:
        return "Cu";
      case Tin:
        return "Sn";
      case Magnesium:
        return "Mg";
      case Air:
        return "  ";
      case Cell:
        return "[]";
      case Moderator:
        return "##";
      case Casing:
        return "--";
      default:
        return "??";
    }
  }
}

std::string stateToString(const xt::xtensor<int, 3> &state) {
  std::string result;
  for (int x{}; x < state.shape(0); ++x) {
    result += "Layer " + std::to_string(x + 1) + "\n";
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        result += Tile::toString(state(x, y, z));
        if (z < state.shape(2) - 1)
          result.push_back(' ');
      }
      result.push_back('\n');
    }
  }
  return result;
}

static int getTileWithCasing(const xt::xtensor<int, 3> &state, int x, int y, int z) {
  if (state.in_bounds(x, y, z))
    return state(x, y, z);
  return Tile::Casing;
}

static xt::xtensor<int, 1> getEffWithCasing(const xt::xtensor<int, 4> &effs, int x, int y, int z) {
  if (effs.in_bounds(x, y, z, 0))
    return xt::view(effs, x, y, z, xt::all());
  return {0, 0};
}

static xt::xtensor<int, 1> hasConnected(const xt::xtensor<int, 3> &state, int x, int y, int z, int dx, int dy, int dz) {
  for (int n{}; n < 5; ++n) {
    x += dx; y += dy; z += dz;
    int tile(getTileWithCasing(state, x, y, z));
    if (tile == Tile::Free) {
      return {0, 1};
    } else if (tile == Tile::Cell) {
      return {1, 1};
    } if (tile != Tile::Moderator) {
      break;
    }
  }
  return {0, 0};
}

static xt::xtensor<int, 1> countEff(const xt::xtensor<int, 3> &state, int x, int y, int z) {
  int tile(state(x, y, z));
  if (tile == Tile::Free || tile == Tile::Cell) {
    xt::xtensor<int, 1> result{1, 1};
    result += hasConnected(state, x, y, z, -1, 0, 0);
    result += hasConnected(state, x, y, z, +1, 0, 0);
    result += hasConnected(state, x, y, z, 0, -1, 0);
    result += hasConnected(state, x, y, z, 0, +1, 0);
    result += hasConnected(state, x, y, z, 0, 0, -1);
    result += hasConnected(state, x, y, z, 0, 0, +1);
    if (tile == Tile::Free)
      result(0) = 0;
    return result;
  }
  return {0, 0};
}

static int countNeighbor(const int neighbors[6], int tile) {
  int result{};
  for (int i{}; i < 6; ++i)
    if (neighbors[i] == tile)
      ++result;
  return result;
}

double evaluate(const Settings &settings, const xt::xtensor<int, 3> &state) {
  double power{}, heat{}, cooling{};
  xt::xtensor<int, 4> effs(xt::empty<int>({settings.sizeX, settings.sizeY, settings.sizeZ, 2}));
  for (int x{}; x < state.shape(0); ++x) {
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        auto eff(xt::view(effs, x, y, z, xt::all()));
        eff = countEff(state, x, y, z);
        power += eff(1);
        heat += eff(0) * (eff(0) + 1) / 2.0;
      }
    }
  }

  for (int x{}; x < state.shape(0); ++x) {
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        int tile(state(x, y, z));
        if (tile == Tile::Free || tile == Tile::Moderator) {
          xt::xtensor<int, 1> eff(
            + getEffWithCasing(effs, x - 1, y, z)
            + getEffWithCasing(effs, x + 1, y, z)
            + getEffWithCasing(effs, x, y - 1, z)
            + getEffWithCasing(effs, x, y + 1, z)
            + getEffWithCasing(effs, x, y, z - 1)
            + getEffWithCasing(effs, x, y, z + 1));
          if (tile == Tile::Free)
            eff(0) = 0;
          else if (!eff(1))
            return -1.0;
          power += eff(1) / 6.0;
          heat += eff(0) / 3.0;
        }
        if (tile == Tile::Free || tile < Tile::Air) {
          int neighbors[] {
            getTileWithCasing(state, x - 1, y, z),
            getTileWithCasing(state, x + 1, y, z),
            getTileWithCasing(state, x, y - 1, z),
            getTileWithCasing(state, x, y + 1, z),
            getTileWithCasing(state, x, y, z - 1),
            getTileWithCasing(state, x, y, z + 1)
          };
          auto isValid([&](int tile) {
            switch (tile) {
              case Tile::Water:
                return countNeighbor(neighbors, Tile::Free)
                  || countNeighbor(neighbors, Tile::Cell)
                  || countNeighbor(neighbors, Tile::Moderator);
              case Tile::Redstone:
                return countNeighbor(neighbors, Tile::Free)
                  || countNeighbor(neighbors, Tile::Cell);
              case Tile::Quartz:
                return countNeighbor(neighbors, Tile::Free)
                  || countNeighbor(neighbors, Tile::Moderator);
              case Tile::Gold:
                return countNeighbor(neighbors, Tile::Free) >= 2
                  - !!countNeighbor(neighbors, Tile::Water)
                  - !! countNeighbor(neighbors, Tile::Redstone);
              case Tile::Glowstone:
                return countNeighbor(neighbors, Tile::Free) >= 2
                  - countNeighbor(neighbors, Tile::Moderator);
              case Tile::Lapis:
                return countNeighbor(neighbors, Tile::Casing)
                  && (countNeighbor(neighbors, Tile::Free) || countNeighbor(neighbors, Tile::Cell));
              case Tile::Diamond:
                return countNeighbor(neighbors, Tile::Free) >= 2
                  - !!countNeighbor(neighbors, Tile::Water)
                  - !! countNeighbor(neighbors, Tile::Quartz);
              case Tile::Helium:
                if (!countNeighbor(neighbors, Tile::Casing)) {
                  return false;
                } else {
                  int nRedstone(countNeighbor(neighbors, Tile::Redstone));
                  if (nRedstone > 1)
                    return false;
                  return nRedstone || countNeighbor(neighbors, Tile::Free);
                }
              case Tile::Enderium:
                return countNeighbor(neighbors, Tile::Casing) == 3;
              case Tile::Cryotheum:
                return countNeighbor(neighbors, Tile::Free) >= 2
                  - countNeighbor(neighbors, Tile::Cell);
              case Tile::Iron:
                return countNeighbor(neighbors, Tile::Free)
                  || countNeighbor(neighbors, Tile::Gold);
              case Tile::Emerald:
                return countNeighbor(neighbors, Tile::Free) >= 2
                  - !!countNeighbor(neighbors, Tile::Moderator)
                  - !! countNeighbor(neighbors, Tile::Cell);
              case Tile::Copper:
                return countNeighbor(neighbors, Tile::Free)
                  || countNeighbor(neighbors, Tile::Glowstone);
              case Tile::Tin:
                for (int i{}; i < 6; i += 2)
                  if ((neighbors[i] == Tile::Free || neighbors[i] == Tile::Lapis)
                      && (neighbors[i + 1] == Tile::Free || neighbors[i + 1] == Tile::Lapis))
                    return true;
                return false;
              case Tile::Magnesium:
                return countNeighbor(neighbors, Tile::Casing)
                  && (countNeighbor(neighbors, Tile::Free) || countNeighbor(neighbors, Tile::Moderator));
              default:
                return false;
            }
          });
          if (tile < Tile::Air) {
            if (!isValid(tile))
              return -1.0;
            cooling += settings.coolingRate[tile];
          } else {
            double best{};
            for (int i{}; i < Tile::Air; ++i) {
              if (settings.allowedCoolers[i] && settings.coolingRate[i] > best && isValid(i)) {
                best = settings.coolingRate[i];
              }
            }
            cooling += best;
          }
        }
      }
    }
  }
  power *= settings.fuelBasePower;
  heat *= settings.fuelBaseHeat;
  return std::min(1., cooling / heat) * power;
}
