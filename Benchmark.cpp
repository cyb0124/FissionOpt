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

  void printSample(const Fission::Sample &sample) {
    std::cout << stateToString(sample.state);
    std::cout << "power=" << sample.value.power << std::endl;
    std::cout << "heat=" << sample.value.heat << std::endl;
    std::cout << "cooling=" << sample.value.cooling << std::endl;
    std::cout << "netHeat=" << sample.value.netHeat << std::endl;
    std::cout << "dutyCycle=" << sample.value.dutyCycle << std::endl;
    std::cout << "avgBreed=" << sample.value.avgBreed << std::endl;
    std::cout << "avgPower=" << sample.value.avgPower << std::endl;
  }

  std::pair<std::string, Fission::Settings> benchmarks[] {
    {"Default, HECf-251-OX, 19*11*5, Power, Passive", {
      19, 11, 5,
      1260, 900,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        60, 90, 90, 120, 130, 120, 150, 140, 120, 160, 80, 160, 80, 120, 110,
        150, 3200, 3000, 4800, 4000, 2800, 7000, 6600, 5400, 6400, 2400, 3600, 2600, 3000, 3600
      },
      true, true, false,
      false, false, false
    }},
    {"Default, HECf-251-OX, 19*11*5, Power, Passive Symmetry", {
      19, 11, 5,
      1260, 900,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        60, 90, 90, 120, 130, 120, 150, 140, 120, 160, 80, 160, 80, 120, 110,
        150, 3200, 3000, 4800, 4000, 2800, 7000, 6600, 5400, 6400, 2400, 3600, 2600, 3000, 3600
      },
      true, true, false,
      true, true, true
    }},
    {"Default, HECf-251-OX, 5*5*5, Breeder, Passive", {
      5, 5, 5,
      1260, 900,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        60, 90, 90, 120, 130, 120, 150, 140, 120, 160, 80, 160, 80, 120, 110,
        150, 3200, 3000, 4800, 4000, 2800, 7000, 6600, 5400, 6400, 2400, 3600, 2600, 3000, 3600
      },
      true, true, true,
      false, false, false
    }},
    {"Default, HECf-251-OX, 5*5*5, Breeder, Passive Symmetry", {
      5, 5, 5,
      1260, 900,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        60, 90, 90, 120, 130, 120, 150, 140, 120, 160, 80, 160, 80, 120, 110,
        150, 3200, 3000, 4800, 4000, 2800, 7000, 6600, 5400, 6400, 2400, 3600, 2600, 3000, 3600
      },
      true, true, true,
      true, true, true
    }},
    {"E2E, LEU-235-OX, 5*5*5, Power, Passive No TE", {
      5, 5, 5,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 9*9*9, Power, Passive No TE", {
      9, 9, 9,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 9*9*9, Efficiency (Max 8 Cells), Passive No TE", {
      9, 9, 9,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 11*11*11, Efficiency (Max 8 Cells), Passive", {
      11, 11, 11,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 5*5*5, Power, Active Glowstone", {
      5, 5, 5,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 1*5*5, Power, Active Glowstone", {
      1, 5, 5,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 5*5*5, Power, Limited Active Glowstone", {
      5, 5, 5,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-235-OX, 5*5*5, Power, Limited Active Helium", {
      5, 5, 5,
      1008, 75,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, LEU-233-OX, 5*5*5, Breeder, Passive", {
      5, 5, 5,
      1209.6, 90,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, true,
      false, false, false
    }},
    {"E2E, LEU-233-OX, 7*7*7, Breeder, Passive", {
      7, 7, 7,
      1209.6, 90,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, true,
      false, false, false
    }},
    {"E2E, HECf-249, 5*5*5, Breeder, Passive", {
      5, 5, 5,
      5184, 835.2,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, true,
      false, false, false
    }},
    {"E2E, HECf-251, 5*5*5, Breeder, Passive", {
      5, 5, 5,
      5400, 864,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, true,
      false, false, false
    }},
    {"E2E, HECf-251, 7*7*7, Breeder, Passive", {
      7, 7, 7,
      5400, 864,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, true,
      false, false, false
    }},
    {"E2E, HECf-249, 5*5*5, Power, Passive", {
      5, 5, 5,
      5184, 835.2,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, HECf-251, 5*5*5, Power, Passive", {
      5, 5, 5,
      5400, 864,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }},
    {"E2E, HECf-251, 7*7*7, Power, Passive", {
      7, 7, 7,
      5400, 864,
      {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1
      },
      {
        20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100,
        50, 1000, 1500, 1750, 2000, 2250, 3500, 3300, 2750, 3250, 1700, 2750, 1125, 1250, 2000
      },
      true, true, false,
      false, false, false
    }}
  };
}

int main() {
  std::string results;
  for (auto &entry : benchmarks) {
    std::cout << entry.first << std::endl;
    Fission::Opt opt(entry.second);
    for (int i{}; i < 1024 * 256; ++i)
      opt.step();
    printSample(opt.getBest());
    std::cout << std::endl;
    results += entry.first + ": " + std::to_string(
      entry.second.breeder ? opt.getBest().value.avgBreed : opt.getBest().value.avgPower) + '\n';
  }
  std::cout << "Results:" << std::endl;
  std::cout << results << std::flush;
}
