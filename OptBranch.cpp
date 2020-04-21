#include "OptBranch.h"

OptBranch::OptBranch(const Settings &settings)
  :settings(settings), bestValue(-1.0),
  nodes{OptBranchNode{xt::broadcast<int>(Tile::Free, {settings.sizeX, settings.sizeY, settings.sizeZ})}} {
  for (int x{}; x < settings.sizeX; ++x) {
    for (int y{}; y < settings.sizeY; ++y) {
      for (int z{}; z < settings.sizeZ; ++z) {
        nodes.back().frees.emplace_back(x, y, z);
      }
    }
  }
  for (int i{}; i < Tile::Air; ++i)
    if (settings.allowedCoolers[i])
      allTiles.emplace_back(i);
  allTiles.emplace_back(Tile::Air);
  allTiles.emplace_back(Tile::Cell);
  allTiles.emplace_back(Tile::Moderator);
}

bool OptBranch::step() {
  OptBranchNode node(std::move(nodes.back()));
  nodes.pop_back();
  double value(evaluate(settings, node.state));
  if (value <= bestValue)
    return false;
  if (node.frees.empty()) {
    bestState = node.state;
    bestValue = value;
    return true;
  }
  int x, y, z;
  std::tie(x, y, z) = node.frees.back();
  node.frees.pop_back();
  for (int tile : allTiles) {
    node.state(x, y, z) = tile;
    nodes.emplace_back(node);
  }
  return false;
}
