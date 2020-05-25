#include "OverhaulFission.h"

namespace OverhaulFission {
  const Coord directions[] {
    {-1, 0, 0},
    {+1, 0, 0},
    {0, -1, 0},
    {0, +1, 0},
    {0, 0, -1},
    {0, 0, +1}
  };

  void Settings::compute() {
    cellTypes.clear();
    for (int i{}; i < static_cast<int>(fuels.size()); ++i) {
      cellTypes.emplace_back(i, 0);
      if (!fuels[i].selfPriming) {
        for (int j(1); j <= 3; ++j) {
          cellTypes.emplace_back(i, j);
        }
      }
    }
  }

  void Evaluation::initialize(const Settings &settings, bool shieldOn) {
    tiles = xt::empty<Tile>({settings.sizeX, settings.sizeY, settings.sizeZ});
    this->settings = &settings;
    this->shieldOn = shieldOn;
  }

  void Evaluation::checkNeutronSource(int x, int y, int z) {
    Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
    if (!cell.neutronSource)
      return;
    for (auto &[dx, dy, dz] : directions) {
      int cx(x), cy(y), cz(z);
      bool blocked{};
      while (!blocked) {
        cx += dx; cy += dy; cz += dz;
        if (!tiles.in_bounds(cx, cy, cz))
          return;
        std::visit(Overload {
          [&](Reflector &tile) { blocked = reflectorFluxMults[tile.type] >= 1.0; },
          [&](Irradiator &tile) { blocked = true; },
          [&](Cell &) { blocked = true; },
          [](...) {}
          // Note: treating active shield as non-blocking because activating shield won't undo the flux activation.
        }, tiles(cx, cy, cz));
      }
    }
    cell.neutronSource = 0;
    cell.isNeutronSourceBlocked = true;
  }

  void Evaluation::computeFluxEdge(int x, int y, int z) {
    Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
    for (int i{}; i < 6; ++i) {
      FluxEdge &edge(cell.fluxEdges[i].emplace());
      auto &[dx, dy, dz](directions[i]);
      int cx(x), cy(y), cz(z);
      bool success{};
      for (edge.nModerators = 0; edge.nModerators <= neutronReach; ++edge.nModerators) {
        cx += dx; cy += dy; cz += dz;
        if (!tiles.in_bounds(cx, cy, cz))
          break;
        bool stop{};
        std::visit(Overload {
          [&](Moderator &tile) {
            edge.efficiency += moderatorEfficiencies[tile.type];
            edge.flux += moderatorFluxes[tile.type];
          },
          [&](Shield &) {
            if (shieldOn) {
              stop = true;
            } else {
              edge.efficiency += shieldEfficiency;
            }
          },
          [&](Cell &) {
            stop = true;
            if (edge.nModerators) {
              edge.efficiency /= edge.nModerators;
              success = true;
            }
          },
          [&](Irradiator &) {
            stop = true;
            if (edge.nModerators) {
              edge.efficiency = 0.0;
              success = true;
            }
          },
          [&](Reflector &tile) {
            stop = true;
            if (edge.nModerators && edge.nModerators <= neutronReach / 2) {
              edge.efficiency = reflectorEfficiencies[tile.type] * edge.efficiency / edge.nModerators;
              edge.flux = static_cast<int>(2 * edge.flux * reflectorFluxMults[tile.type]);
              edge.isReflected = true;
              success = true;
            }
          },
          [&](...) {
            stop = true;
          }
        }, tiles(cx, cy, cz));
        if (stop) {
          break;
        }
      }
      if (!success) {
        cell.fluxEdges[i].reset();
      }
    }
  }

  void Evaluation::propagateFlux(int x, int y, int z) {
    Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
    if (cell.hasAlreadyPropagatedFlux)
      return;
    cell.hasAlreadyPropagatedFlux = true;
    for (int i{}; i < 6; ++i) {
      if (!cell.fluxEdges[i].has_value())
        continue;
      FluxEdge &edge(*cell.fluxEdges[i]);
      auto &[dx, dy, dz] = directions[i];
      int cx(x + dx * edge.nModerators);
      int cy(y + dy * edge.nModerators);
      int cz(z + dz * edge.nModerators);
      Cell *to(std::get_if<Cell>(&tiles(cx, cy, cz)));
      if (!to)
        continue;
      to->flux += edge.flux;
      if (to->flux >= to->fuel.criticality) {
        propagateFlux(cx, cy, cz);
      }
    }
  }

  void Evaluation::propagateFlux() {
    bool converged{};
    while (!converged) {
      fluxRoots.clear();
      for (auto &[x, y, z] : cells) {
        Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
        // Note: neutron source redirection is not supported as it will cause non-functioning blocks
        // to have an effect on the reactor's behavior, causing the canonicalization to incorrectly
        // remove blocks.
        if (!cell.isExcludedFromFluxRoots && (cell.fuel.selfPriming || cell.neutronSource))
          fluxRoots.emplace_back(x, y, z);
        cell.hasAlreadyPropagatedFlux = false;
        cell.flux = 0;
      }
      for (auto &[x, y, z] : fluxRoots)
        propagateFlux(x, y, z);
      converged = true;
      for (auto &[x, y, z] : fluxRoots) {
        Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
        if (cell.flux < cell.fuel.criticality) {
          cell.isExcludedFromFluxRoots = true;
          converged = false;
        }
      }
    }
  }

  void Evaluation::computeFluxActivation() {
    for (auto &[x, y, z] : cells) {
      Cell &cell(*std::get_if<Cell>(&tiles(x, y, z)));
      cell.isActive = cell.flux >= cell.fuel.criticality;
      for (int i{}; i < 6; ++i) {
        if (!cell.fluxEdges[i].has_value())
          continue;
        FluxEdge &edge(*cell.fluxEdges[i]);
        ++cell.heatMult;
        cell.efficiency += edge.efficiency;
        auto &[dx, dy, dz] = directions[i];
        int cx(x), cy(y), cz(z);
        for (int j{}; j <= edge.nModerators; ++j) {
          cx += dx; cy += dy; cz += dz;
          std::visit(Overload {
            [&](Moderator &tile) {
              if (!j)
                tile.isActive = true;
              tile.isFunctional = true;
            },
            [&](Shield &tile) {
              if (edge.isReflected || i & 1)
                tile.heat += edge.flux * shieldHeatPerFlux;
              tile.isFunctional = true;
            },
            [&](Irradiator &tile) {
              tile.isActive = true;
              tile.flux += edge.flux;
            },
            [&](Reflector &tile) {
              tile.isActive = true;
            },
            [&](...) { }
          }, tiles(cx, cy, cz));
        }
      }
    }
  }

  int Evaluation::countAdjacentCells(int x, int y, int z) {
    int result{};
    for (auto &[dx, dy, dz] : directions) {
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        Cell *tile(std::get_if<Cell>(&tiles(cx, cy, cz)));
        result += tile && tile->isActive;
      }
    }
    return result;
  }

  int Evaluation::countAdjacentCasings(int x, int y, int z) {
    int result{};
    for (auto &[dx, dy, dz] : directions)
      result += !tiles.in_bounds(x + dx, y + dy, z + dz);
    return result;
  }

  int Evaluation::countAdjacentReflectors(int x, int y, int z) {
    int result{};
    for (auto &[dx, dy, dz] : directions) {
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        Reflector *tile(std::get_if<Reflector>(&tiles(cx, cy, cz)));
        result += tile && tile->isActive;
      }
    }
    return result;
  }

  int Evaluation::countAdjacentModerators(int x, int y, int z) {
    int result{};
    for (auto &[dx, dy, dz] : directions) {
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        Moderator *tile(std::get_if<Moderator>(&tiles(cx, cy, cz)));
        result += tile && tile->isActive;
      }
    }
    return result;
  }

  int Evaluation::countAdjacentHeatSinks(int type, int x, int y, int z) {
    int result{};
    for (auto &[dx, dy, dz] : directions) {
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        HeatSink *tile(std::get_if<HeatSink>(&tiles(cx, cy, cz)));
        result += tile && tile->isActive && tile->type == type;
      }
    }
    return result;
  }

  bool Evaluation::hasAxialAdjacentHeatSinks(int type, int x, int y, int z) {
    for (int i{}; i < 6; ++i) {
      bool valid{};
      auto &[dx, dy, dz](directions[i]);
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        HeatSink *tile(std::get_if<HeatSink>(&tiles(cx, cy, cz)));
        valid = tile && tile->isActive && tile->type == type;
      }
      if (i & 1) {
        if (valid) {
          return true;
        }
      } else {
        i |= !valid;
      }
    }
    return false;
  }

  bool Evaluation::hasAxialAdjacentReflectors(int x, int y, int z) {
    for (int i{}; i < 6; ++i) {
      bool valid{};
      auto &[dx, dy, dz](directions[i]);
      int cx(x + dx), cy(y + dy), cz(z + dz);
      if (tiles.in_bounds(cx, cy, cz)) {
        Reflector *tile(std::get_if<Reflector>(&tiles(cx, cy, cz)));
        valid = tile && tile->isActive;
      }
      if (i & 1) {
        if (valid) {
          return true;
        }
      } else {
        i |= !valid;
      }
    }
    return false;
  }

  void Evaluation::computeHeatSinkActivation(int x, int y, int z) {
    HeatSink &tile(*std::get_if<HeatSink>(&tiles(x, y, z)));
    switch (tile.type) {
      default: // Wt
        tile.isActive = countAdjacentCells(x, y, z);
        break;
      case Tiles::Fe:
        tile.isActive = countAdjacentModerators(x, y, z);
        break;
      case Tiles::Rs:
        tile.isActive = countAdjacentCells(x, y, z) && countAdjacentModerators(x, y, z);
        break;
      case Tiles::Qz:
        tile.isActive = countAdjacentHeatSinks(Tiles::Rs, x, y, z);
        break;
      case Tiles::Ob:
        tile.isActive = hasAxialAdjacentHeatSinks(Tiles::Gs, x, y, z);
        break;
      case Tiles::Nr:
        tile.isActive = countAdjacentHeatSinks(Tiles::Ob, x, y, z);
        break;
      case Tiles::Gs:
        tile.isActive = countAdjacentModerators(x, y, z) >= 2;
        break;
      case Tiles::Lp:
        tile.isActive = countAdjacentCells(x, y, z) && countAdjacentCasings(x, y, z);
        break;
      case Tiles::Au:
        tile.isActive = countAdjacentHeatSinks(Tiles::Fe, x, y, z) >= 2;
        break;
      case Tiles::Pm:
        tile.isActive = countAdjacentHeatSinks(Tiles::Wt, x, y, z) >= 2;
        break;
      case Tiles::Sm:
        tile.isActive = countAdjacentHeatSinks(Tiles::Wt, x, y, z) == 1 && countAdjacentHeatSinks(Tiles::Pb, x, y, z) >= 2;
        break;
      case Tiles::En:
        tile.isActive = countAdjacentReflectors(x, y, z);
        break;
      case Tiles::Pr:
        tile.isActive = countAdjacentHeatSinks(Tiles::Fe, x, y, z) == 1 && countAdjacentHeatSinks(Tiles::En, x, y, z);
        break;
      case Tiles::Dm:
        tile.isActive = countAdjacentHeatSinks(Tiles::Au, x, y, z) && countAdjacentCells(x, y, z);
        break;
      case Tiles::Em:
        tile.isActive = countAdjacentHeatSinks(Tiles::Pm, x, y, z) && countAdjacentModerators(x, y, z);
        break;
      case Tiles::Cu:
        tile.isActive = countAdjacentHeatSinks(Tiles::Wt, x, y, z);
        break;
      case Tiles::Sn:
        tile.isActive = hasAxialAdjacentHeatSinks(Tiles::Lp, x, y, z);
        break;
      case Tiles::Pb:
        tile.isActive = countAdjacentHeatSinks(Tiles::Fe, x, y, z);
        break;
      case Tiles::B:
        tile.isActive = countAdjacentHeatSinks(Tiles::Qz, x, y, z) == 1 && countAdjacentCasings(x, y, z);
        break;
      case Tiles::Li:
        tile.isActive = hasAxialAdjacentHeatSinks(Tiles::Pb, x, y, z)
          && countAdjacentHeatSinks(Tiles::Pb, x, y, z) == 2 && countAdjacentCasings(x, y, z);
        break;
      case Tiles::Mg:
        tile.isActive = countAdjacentModerators(x, y, z) == 1 && countAdjacentCasings(x, y, z);
        break;
      case Tiles::Mn:
        tile.isActive = countAdjacentCells(x, y, z) >= 2;
        break;
      case Tiles::Al:
        tile.isActive = countAdjacentHeatSinks(Tiles::Qz, x, y, z) && countAdjacentHeatSinks(Tiles::Lp, x, y, z);
        break;
      case Tiles::Ag:
        tile.isActive = countAdjacentHeatSinks(Tiles::Gs, x, y, z) >= 2 && countAdjacentHeatSinks(Tiles::Sn, x, y, z);
        break;
      case Tiles::Fl:
        tile.isActive = countAdjacentHeatSinks(Tiles::Au, x, y, z) && countAdjacentHeatSinks(Tiles::Pm, x, y, z);
        break;
      case Tiles::Vi:
        tile.isActive = countAdjacentHeatSinks(Tiles::En, x, y, z) && countAdjacentHeatSinks(Tiles::Rs, x, y, z);
        break;
      case Tiles::Cb:
        tile.isActive = countAdjacentHeatSinks(Tiles::Cu, x, y, z) && countAdjacentHeatSinks(Tiles::En, x, y, z);
        break;
      case Tiles::As:
        tile.isActive = hasAxialAdjacentReflectors(x, y, z);
        break;
      case Tiles::N:
        tile.isActive = countAdjacentHeatSinks(Tiles::Cu, x, y, z) >= 2 && countAdjacentHeatSinks(Tiles::Pr, x, y, z);
        break;
      case Tiles::He:
        tile.isActive = countAdjacentHeatSinks(Tiles::Rs, x, y, z) == 2 && countAdjacentCasings(x, y, z);
        break;
      case Tiles::Ed:
        tile.isActive = countAdjacentModerators(x, y, z) >= 3;
        break;
      case Tiles::Cr:
        tile.isActive = countAdjacentCells(x, y, z) >= 3;
    }
  }

  void Evaluation::run(const State &state) {
    cells.clear();
    tier1s.clear();
    tier2s.clear();
    tier3s.clear();
    for (int x{}; x < settings->sizeX; ++x) {
      for (int y{}; y < settings->sizeY; ++y) {
        for (int z{}; z < settings->sizeZ; ++z) {
          Tile &tile(tiles(x, y, z));
          int type(state(x, y, z));
          if (type < Tiles::M0) {
            tile.emplace<HeatSink>(type);
            switch (type) {
              case Tiles::Wt:
              case Tiles::Fe:
              case Tiles::Rs:
              case Tiles::Gs:
              case Tiles::Lp:
              case Tiles::En:
              case Tiles::Mg:
              case Tiles::Mn:
              case Tiles::As:
              case Tiles::Ed:
              case Tiles::Cr:
                tier1s.emplace_back(x, y, z);
                break;
              case Tiles::Qz:
              case Tiles::Ob:
              case Tiles::Au:
              case Tiles::Pm:
              case Tiles::Pr:
              case Tiles::Cu:
              case Tiles::Sn:
              case Tiles::Pb:
              case Tiles::Vi:
              case Tiles::He:
                tier2s.emplace_back(x, y, z);
                break;
              default:
                tier3s.emplace_back(x, y, z);
            }
          } else if (type < Tiles::R0) {
            tile.emplace<Moderator>(type - Tiles::M0);
          } else if (type < Tiles::Shield) {
            tile.emplace<Reflector>(type - Tiles::R0);
          } else switch (type) {
            case Tiles::Shield:
              tile.emplace<Shield>();
              break;
            case Tiles::Conductor:
              tile.emplace<Conductor>();
              break;
            case Tiles::Irradiator:
              tile.emplace<Irradiator>();
              break;
            case Tiles::Air:
              tile.emplace<Air>();
              break;
            default:
              auto &cellType(settings->cellTypes[type - Tiles::C0]);
              tile.emplace<Cell>(settings->fuels[cellType.first], cellType.second);
              cells.emplace_back(x, y, z);
          }
        }
      }
    }
    for (auto &[x, y, z] : cells) {
      checkNeutronSource(x, y, z);
      computeFluxEdge(x, y, z);
    }
    propagateFlux();
    computeFluxActivation();
    for (auto &[x, y, z] : tier1s)
      computeHeatSinkActivation(x, y, z);
    for (auto &[x, y, z] : tier2s)
      computeHeatSinkActivation(x, y, z);
    for (auto &[x, y, z] : tier3s)
      computeHeatSinkActivation(x, y, z);
    // TODO: form clusters & conductor groups
    // TODO: compute stats
  }
}
