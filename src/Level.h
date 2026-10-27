#pragma once

#include "GameTypes.h"

namespace tri {
// Holds the map and its permanent cargo stations
class Level {
public:
    // Every spot on the grid has one kind of ground
    using Grid = std::array<std::array<Tile, GridWidth>, GridHeight>;

    explicit Level(LevelDefinition definition);

    // Put the map back the way it was for another try
    void Reset();

    // Handy checks used by minion movement and pathfinding
    bool IsInside(int x, int y) const;
    bool IsPathable(int x, int y) const;
    int Distance(CellPos a, CellPos b) const;
    Tile At(int x, int y) const;
    bool IsInsideCargoArea(CellPos cell, CellPos station) const;

    // Let other parts of the game look at the map
    const Grid& Tiles() const { return tiles_; }
    const LevelDefinition& Definition() const { return definition_; }
    std::vector<CellPos> CargoStations() const;
    CellPos Start() const { return definition_.start; }

private:
    // Save a clean copy so a restart can put everything back
    LevelDefinition definition_;
    Grid tiles_{};
};
}
