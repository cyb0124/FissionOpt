#include <xtensor/xnpy.hpp>
#include "OverhaulFissionNet.h"

int main() {
  OverhaulFission::Settings settings {
    7, 7, 7,
    {
      {1.75, -1, 60, 540, true},
      {1.75, -1, 75, 432, true},
      {1.75, -1, 51, 676, true},
      {1.80, -1, 30, 1620, true},
      {1.80, -1, 37, 1296, true},
      {1.80, -1, 25, 2028, true},
      {1.80, -1, 71, 288, true},
      {1.80, -1, 89, 230, true},
      {1.80, -1, 60, 360, true},
      {1.85, -1, 35, 864, true},
      {1.85, -1, 44, 690, true},
      {1.85, -1, 30, 1080, true}
    },
    {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1
    },
    {0, 0, 0},
    OverhaulFission::GoalOutput,
    false, false, true, true
  };
  OverhaulFission::Opt opt(settings);
  while (true) {
    opt.stepInteractive();
    if (opt.needsRedrawBest()) {
      std::cout << "output: " << opt.getBest().value.output / 16 << std::endl;
      xt::dump_npy("best.npy", opt.getBest().state);
    }
  }
}
