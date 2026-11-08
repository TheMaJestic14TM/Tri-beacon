#pragma once

#include "GameTypes.h"

namespace tri {
// Keeps the map tiles and cargo stations in one place
class Level {
public:
    // The board is a fixed grid of game tiles
    using Grid = std::array<std::array<Tile, GridWidth>, GridHeight>;

    explicit Level(LevelDefinition definition);

    // Restore the map before a new run
    void Reset();

    // Basic checks used by movement and pathfinding
    bool IsInside(int x, int y) const;
    bool IsPathable(int x, int y) const;
    int Distance(CellPos a, CellPos b) const;
    Tile At(int x, int y) const;

    // Let the game and renderer read the map data
    const Grid& Tiles() const { return tiles_; }
    const LevelDefinition& Definition() const { return definition_; }
    const std::vector<CellPos>& CargoStations() const { return cargoStations_; }
    CellPos Start() const { return definition_.start; }

private:
    // Keep the original map so Reset can restore it
    LevelDefinition definition_;
    Grid tiles_{};
    std::vector<CellPos> cargoStations_;
};
}
