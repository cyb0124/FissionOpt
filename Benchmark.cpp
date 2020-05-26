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
    true, true, true
  };
  settings.compute();
  OverhaulFission::State state(xt::broadcast<int>(OverhaulFission::Tiles::Air, {5, 5, 5}));
  state(0, 0, 0) = OverhaulFission::Tiles::C0 + 1;
  state(1, 0, 0) = OverhaulFission::Tiles::M2;
  state(2, 0, 0) = OverhaulFission::Tiles::M2;
  state(3, 0, 0) = OverhaulFission::Tiles::M2;
  state(4, 0, 0) = OverhaulFission::Tiles::C0 + 1;
  OverhaulFission::Evaluation eval;
  eval.initialize(settings, false);
  eval.run(state);
  std::cout << eval.nActiveCells << std::endl;
}
