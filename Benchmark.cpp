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
  state(0, 0, 0) = OverhaulFission::Tiles::Cu;
  state(0, 0, 1) = OverhaulFission::Tiles::Wt;
  state(0, 0, 2) = OverhaulFission::Tiles::Fe;
  state(0, 0, 3) = OverhaulFission::Tiles::Wt;
  state(0, 0, 4) = OverhaulFission::Tiles::Cu;
  state(0, 1, 0) = OverhaulFission::Tiles::Lp;
  state(0, 1, 1) = OverhaulFission::Tiles::Mg;
  state(0, 1, 2) = OverhaulFission::Tiles::Pb;
  state(0, 1, 3) = OverhaulFission::Tiles::Mg;
  state(0, 1, 4) = OverhaulFission::Tiles::Lp;
  state(0, 2, 0) = OverhaulFission::Tiles::Sn;
  state(0, 2, 1) = OverhaulFission::Tiles::Lp;
  state(0, 2, 2) = OverhaulFission::Tiles::Sn;
  state(0, 2, 3) = OverhaulFission::Tiles::Lp;
  state(0, 2, 4) = OverhaulFission::Tiles::Sn;
  state(0, 3, 0) = OverhaulFission::Tiles::Lp;
  state(0, 3, 1) = OverhaulFission::Tiles::Mg;
  state(0, 3, 2) = OverhaulFission::Tiles::Pb;
  state(0, 3, 3) = OverhaulFission::Tiles::Mg;
  state(0, 3, 4) = OverhaulFission::Tiles::Lp;
  state(0, 4, 0) = OverhaulFission::Tiles::Cu;
  state(0, 4, 1) = OverhaulFission::Tiles::Wt;
  state(0, 4, 2) = OverhaulFission::Tiles::Fe;
  state(0, 4, 3) = OverhaulFission::Tiles::Wt;
  state(0, 4, 4) = OverhaulFission::Tiles::Cu;

  state(1, 0, 0) = OverhaulFission::Tiles::Mn;
  state(1, 0, 1) = OverhaulFission::Tiles::C0 + 1;
  state(1, 0, 2) = OverhaulFission::Tiles::M2;
  state(1, 0, 3) = OverhaulFission::Tiles::C0 + 1;
  state(1, 0, 4) = OverhaulFission::Tiles::Mn;
  state(1, 1, 0) = OverhaulFission::Tiles::C0 + 1;
  state(1, 1, 1) = OverhaulFission::Tiles::M2;
  state(1, 1, 2) = OverhaulFission::Tiles::M0;
  state(1, 1, 3) = OverhaulFission::Tiles::M2;
  state(1, 1, 4) = OverhaulFission::Tiles::C0 + 1;
  state(1, 2, 0) = OverhaulFission::Tiles::M0;
  state(1, 2, 1) = OverhaulFission::Tiles::C0 + 1;
  state(1, 2, 2) = OverhaulFission::Tiles::M2;
  state(1, 2, 3) = OverhaulFission::Tiles::C0 + 1;
  state(1, 2, 4) = OverhaulFission::Tiles::M0;
  state(1, 3, 0) = OverhaulFission::Tiles::C0 + 1;
  state(1, 3, 1) = OverhaulFission::Tiles::M2;
  state(1, 3, 2) = OverhaulFission::Tiles::M0;
  state(1, 3, 3) = OverhaulFission::Tiles::M2;
  state(1, 3, 4) = OverhaulFission::Tiles::C0 + 1;
  state(1, 4, 0) = OverhaulFission::Tiles::Mn;
  state(1, 4, 1) = OverhaulFission::Tiles::C0 + 1;
  state(1, 4, 2) = OverhaulFission::Tiles::M2;
  state(1, 4, 3) = OverhaulFission::Tiles::C0 + 1;
  state(1, 4, 4) = OverhaulFission::Tiles::Mn;

  state(2, 0, 0) = OverhaulFission::Tiles::Gs;
  state(2, 0, 1) = OverhaulFission::Tiles::M2;
  state(2, 0, 3) = OverhaulFission::Tiles::M2;
  state(2, 0, 4) = OverhaulFission::Tiles::Gs;
  state(2, 1, 0) = OverhaulFission::Tiles::M0;
  state(2, 1, 1) = OverhaulFission::Tiles::Ed;
  state(2, 1, 3) = OverhaulFission::Tiles::Ed;
  state(2, 1, 4) = OverhaulFission::Tiles::M0;
  state(2, 2, 0) = OverhaulFission::Tiles::Ed;
  state(2, 2, 1) = OverhaulFission::Tiles::Mn;
  state(2, 2, 3) = OverhaulFission::Tiles::Mn;
  state(2, 2, 4) = OverhaulFission::Tiles::Ed;
  state(2, 3, 0) = OverhaulFission::Tiles::M0;
  state(2, 3, 1) = OverhaulFission::Tiles::Ed;
  state(2, 3, 3) = OverhaulFission::Tiles::Ed;
  state(2, 3, 4) = OverhaulFission::Tiles::M0;
  state(2, 4, 0) = OverhaulFission::Tiles::Gs;
  state(2, 4, 1) = OverhaulFission::Tiles::M2;
  state(2, 4, 3) = OverhaulFission::Tiles::M2;
  state(2, 4, 4) = OverhaulFission::Tiles::Gs;

  state(3, 0, 0) = OverhaulFission::Tiles::Mn;
  state(3, 0, 1) = OverhaulFission::Tiles::C0 + 1;
  state(3, 0, 2) = OverhaulFission::Tiles::M2;
  state(3, 0, 3) = OverhaulFission::Tiles::C0 + 1;
  state(3, 0, 4) = OverhaulFission::Tiles::Mn;
  state(3, 1, 0) = OverhaulFission::Tiles::C0 + 1;
  state(3, 1, 1) = OverhaulFission::Tiles::M2;
  state(3, 1, 2) = OverhaulFission::Tiles::M0;
  state(3, 1, 3) = OverhaulFission::Tiles::M2;
  state(3, 1, 4) = OverhaulFission::Tiles::C0 + 1;
  state(3, 2, 0) = OverhaulFission::Tiles::M0;
  state(3, 2, 1) = OverhaulFission::Tiles::C0 + 1;
  state(3, 2, 2) = OverhaulFission::Tiles::M2;
  state(3, 2, 3) = OverhaulFission::Tiles::C0 + 1;
  state(3, 2, 4) = OverhaulFission::Tiles::M0;
  state(3, 3, 0) = OverhaulFission::Tiles::C0 + 1;
  state(3, 3, 1) = OverhaulFission::Tiles::M2;
  state(3, 3, 2) = OverhaulFission::Tiles::M0;
  state(3, 3, 3) = OverhaulFission::Tiles::M2;
  state(3, 3, 4) = OverhaulFission::Tiles::C0 + 1;
  state(3, 4, 0) = OverhaulFission::Tiles::Mn;
  state(3, 4, 1) = OverhaulFission::Tiles::C0 + 1;
  state(3, 4, 2) = OverhaulFission::Tiles::M2;
  state(3, 4, 3) = OverhaulFission::Tiles::C0 + 1;
  state(3, 4, 4) = OverhaulFission::Tiles::Mn;

  state(4, 0, 0) = OverhaulFission::Tiles::Cu;
  state(4, 0, 1) = OverhaulFission::Tiles::Wt;
  state(4, 0, 2) = OverhaulFission::Tiles::Fe;
  state(4, 0, 3) = OverhaulFission::Tiles::Wt;
  state(4, 0, 4) = OverhaulFission::Tiles::Cu;
  state(4, 1, 0) = OverhaulFission::Tiles::Lp;
  state(4, 1, 1) = OverhaulFission::Tiles::Mg;
  state(4, 1, 2) = OverhaulFission::Tiles::Pb;
  state(4, 1, 3) = OverhaulFission::Tiles::Mg;
  state(4, 1, 4) = OverhaulFission::Tiles::Lp;
  state(4, 2, 0) = OverhaulFission::Tiles::Sn;
  state(4, 2, 1) = OverhaulFission::Tiles::Lp;
  state(4, 2, 2) = OverhaulFission::Tiles::Sn;
  state(4, 2, 3) = OverhaulFission::Tiles::Lp;
  state(4, 2, 4) = OverhaulFission::Tiles::Sn;
  state(4, 3, 0) = OverhaulFission::Tiles::Lp;
  state(4, 3, 1) = OverhaulFission::Tiles::Mg;
  state(4, 3, 2) = OverhaulFission::Tiles::Pb;
  state(4, 3, 3) = OverhaulFission::Tiles::Mg;
  state(4, 3, 4) = OverhaulFission::Tiles::Lp;
  state(4, 4, 0) = OverhaulFission::Tiles::Cu;
  state(4, 4, 1) = OverhaulFission::Tiles::Wt;
  state(4, 4, 2) = OverhaulFission::Tiles::Fe;
  state(4, 4, 3) = OverhaulFission::Tiles::Wt;
  state(4, 4, 4) = OverhaulFission::Tiles::Cu;
  OverhaulFission::Evaluation eval;
  eval.initialize(settings, false);
  eval.run(state);
  std::cout << eval.output / 16.0 << std::endl;
}
