#ifndef _OPT_BRANCH_H_
#define _OPT_BRANCH_H_
#include "Fission.h"

struct OptBranchNode {
  xt::xtensor<int, 3> state;
  std::vector<std::tuple<int, int, int>> frees;
};

class OptBranch {
  const Settings &settings;
  xt::xtensor<int, 3> bestState;
  double bestValue;
  std::vector<OptBranchNode> nodes;
  std::vector<int> allTiles;
public:
  OptBranch(const Settings &settings);
  bool isFinished() const { return nodes.empty(); }
  bool step();
  const xt::xtensor<int, 3> &getBestState() const { return bestState; };
  double getBestValue() const { return bestValue; }
};

#endif
