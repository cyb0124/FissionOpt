#include <iostream>
#include "OptOverhaulFission.h"

int main() {
  OverhaulFission::Settings settings {
    5, 5, 5,
    {{1.0, -1, 102, 120, false}},
    {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1
    },
    {-1, 0, 0},
    OverhaulFission::GoalOutput,
    false,
    false, false, false
  };
  OverhaulFission::Opt opt(settings);
  for (int i{}; i < 100'000; ++i)
    opt.step();
}
