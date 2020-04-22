#include <iostream>
#include "OptMeta.h"

static std::string tileToString(int tile) {
  switch (tile) {
    case Tile::Water:
      return "Wt";
    case Tile::Redstone:
      return "Rs";
    case Tile::Quartz:
      return "Qz";
    case Tile::Gold:
      return "Au";
    case Tile::Glowstone:
      return "Gs";
    case Tile::Lapis:
      return "Lp";
    case Tile::Diamond:
      return "Dm";
    case Tile::Helium:
      return "He";
    case Tile::Enderium:
      return "Ed";
    case Tile::Cryotheum:
      return "Cr";
    case Tile::Iron:
      return "Fe";
    case Tile::Emerald:
      return "Em";
    case Tile::Copper:
      return "Cu";
    case Tile::Tin:
      return "Sn";
    case Tile::Magnesium:
      return "Mg";
    case Tile::Air:
      return "  ";
    case Tile::Cell:
      return "[]";
    case Tile::Moderator:
      return "##";
    default:
      return "??";
  }
}

static std::string stateToString(const xt::xtensor<int, 3> &state) {
  std::string result;
  for (int x{}; x < state.shape(0); ++x) {
    result += "Layer " + std::to_string(x + 1) + "\n";
    for (int y{}; y < state.shape(1); ++y) {
      for (int z{}; z < state.shape(2); ++z) {
        result += tileToString(state(x, y, z));
        if (z < state.shape(2) - 1)
          result.push_back(' ');
      }
      result.push_back('\n');
    }
  }
  return result;
}

static void printIndividual(const OptMetaIndividual &individual) {
  std::cout << stateToString(individual.state);
  std::cout << "power=" << individual.value.power << std::endl;
  std::cout << "heat=" << individual.value.heat << std::endl;
  std::cout << "cooling=" << individual.value.cooling << std::endl;
  std::cout << "netHeat=" << individual.value.netHeat() << std::endl;
  std::cout << "dutyCycle=" << individual.value.dutyCycle() << std::endl;
  std::cout << "effPower=" << individual.value.effPower() << std::endl;
}

int main() {
  Settings settings{
    1, 3, 5,
    682.105263158, 56.8421052632,
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100}
  };
  OptMeta opt(settings);
  while (true) {
    int whichChanged(opt.step());
    if (whichChanged & 1) {
      std::cout << "*** Best ***" << std::endl;
      printIndividual(opt.getBest());
    }
    if (whichChanged & 2) {
      std::cout << "*** Best No Net Heat ***" << std::endl;
      printIndividual(opt.getBestNoNetHeat());
    }
  }
}
