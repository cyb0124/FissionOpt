#include <iostream>
#include "OptFission.h"

namespace {
  std::string tileToString(int tile) {
    switch (tile) {
      case Fission::Water:
        return "Wt";
      case Fission::Redstone:
        return "Rs";
      case Fission::Quartz:
        return "Qz";
      case Fission::Gold:
        return "Au";
      case Fission::Glowstone:
        return "Gs";
      case Fission::Lapis:
        return "Lp";
      case Fission::Diamond:
        return "Dm";
      case Fission::Helium:
        return "He";
      case Fission::Enderium:
        return "Ed";
      case Fission::Cryotheum:
        return "Cr";
      case Fission::Iron:
        return "Fe";
      case Fission::Emerald:
        return "Em";
      case Fission::Copper:
        return "Cu";
      case Fission::Tin:
        return "Sn";
      case Fission::Magnesium:
        return "Mg";
      case Fission::Cell:
        return "[]";
      case Fission::Moderator:
        return "##";
      default:
        return "  ";
    }
  }

  std::string stateToString(const xt::xtensor<int, 3> &state) {
    std::string result;
    for (int x{}; x < state.shape(0); ++x) {
      result += "Layer " + std::to_string(x + 1) + "\n";
      for (int y{}; y < state.shape(1); ++y) {
        for (int z{}; z < state.shape(2); ++z) {
          int tile(state(x, y, z));
          if (tile >= Fission::Active && tile < Fission::Cell)
            result += ">" + tileToString(tile - Fission::Active);
          else
            result += " " + tileToString(tile);
        }
        result.push_back('\n');
      }
    }
    return result;
  }

  void printIndividual(const Fission::Individual &individual) {
    std::cout << stateToString(individual.state);
    std::cout << "power=" << individual.value.power << std::endl;
    std::cout << "heat=" << individual.value.heat << std::endl;
    std::cout << "cooling=" << individual.value.cooling << std::endl;
    std::cout << "netHeat=" << individual.value.netHeat << std::endl;
    std::cout << "dutyCycle=" << individual.value.dutyCycle << std::endl;
    std::cout << "avgPower=" << individual.value.avgPower << std::endl;
    std::cout << "avgBreed=" << individual.value.avgBreed << std::endl;
    std::cout << "efficiency=" << individual.value.efficiency << std::endl;
  }
}

int main() {
  Fission::Settings settings{
    5, 5, 5,
    682.105263158, 56.8421052632,
    {
      -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1,
       0,  8,  0,  0,  0,  0,  0,  0, 0, 0, 0,  0,  0,  0,  0,
      -1, -1
    },
    {
      20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
      50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
    },
    true
  };
  settings.bestPower(true);
  Fission::Opt opt(settings);
  while (true) {
    if (opt.step()) {
      std::cout << "*** Best ***" << std::endl;
      printIndividual(opt.getBest());
    }
  }
}
