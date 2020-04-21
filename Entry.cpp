#include <iostream>
#include "OptMeta.h"

static void printIndividual(const OptMetaIndividual &individual) {
  std::cout << stateToString(individual.state);
  std::cout << "power=" << individual.value.power << std::endl;
  std::cout << "heat=" << individual.value.heat << std::endl;
  std::cout << "cooling=" << individual.value.cooling << std::endl;
  std::cout << "netHeat=" << individual.value.netHeat() << std::endl;
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
  for (int nRestart{}; ; ++nRestart) {
    for (int nGeneration{}; nGeneration < 1000; ++nGeneration) {
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
    opt.restart();
  }
}
