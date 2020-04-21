#include <iostream>
#include "OptMeta.h"

int main() {
  Settings settings{
    1, 3, 5,
    136, 56,
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100}
  };
  OptMeta opt(settings);
  for (int nRestart{}; ; ++nRestart) {
    for (int nGeneration{}; nGeneration < 1000; ++nGeneration) {
      if (opt.step()) {
        std::cout << stateToString(opt.getBestState());
        std::cout << "bestValue=" << opt.getBestValue() << std::endl;
      }
    }
    opt.restart();
  }
}
