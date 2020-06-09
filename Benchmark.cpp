#include <iostream>
#include "OverhaulFissionNet.h"

int main() {
  OverhaulFission::Settings settings {
    6, 7, 8,
    {
      {1.35, 10, 65, 390, false},
      {1.40, -1, 32, 1170, false}
    },
    {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1
    },
    {-1, 0, 0},
    OverhaulFission::GoalOutput,
    false, false, false, true
  };
  OverhaulFission::Opt opt(settings);
  while (true) {
    opt.stepInteractive();
    if (opt.needsRedrawBest()) {
      std::cout << "output: " << opt.getBest().value.output / 16 << std::endl;
      std::cout << "irrFlux: " << opt.getBest().value.irradiatorFlux << std::endl;
    }
  }
}
