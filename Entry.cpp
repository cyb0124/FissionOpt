#include <iostream>
#include "OptMeta.h"

int main() {
  Settings settings{
    1, 3, 5,
    136 * 6.0, 56 * 1.2,
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100}
  };
  OptMeta opt(settings);
  for (int nRestart{}; ; ++nRestart) {
    for (int nGeneration{}; nGeneration < 1000; ++nGeneration) {
      if (opt.step()) {
        std::cout << stateToString(opt.getBestSoFar().state);
        std::cout << "power=" << opt.getBestSoFar().value.power << std::endl;
        std::cout << "heat=" << opt.getBestSoFar().value.heat << std::endl;
        std::cout << "cooling=" << opt.getBestSoFar().value.cooling << std::endl;
        std::cout << "netHeat=" << opt.getBestSoFar().value.netHeat() << std::endl;
        std::cout << "effPower=" << opt.getBestSoFar().value.effPower() << std::endl;
      }
    }
    opt.restart();
  }
}
