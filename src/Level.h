#pragma once

#include "GameTypes.h"

namespace tri {
// Stores the map and tracks each cargo station
class Level {
public:
    // Every spot on the grid has one kind of ground
    using Grid = std::array<std::array<Tile, GridWidth>, GridHeight>;

    explicit Level(LevelDefinition definition);

    // Put the map back the way it was for another try
    void Reset();

    // Small map checks used for movement and routes
    bool IsInside(int x, int y) const;
    bool IsPathable(int x, int y) const;
    int Distance(CellPos a, CellPos b) const;
    Tile At(int x, int y) const;
    bool IsCargoAvailable(int stationIndex) const;
    bool ClaimCargoStation(int stationIndex);
    void ReleaseCargoStation(int stationIndex);
    void DeliverCargoStation(int stationIndex);

    // Let other parts of the game look at the map
    const Grid& Tiles() const { return tiles_; }
    const LevelDefinition& Definition() const { return definition_; }
    const std::vector<CellPos>& CargoStations() const { return cargoStations_; }
    CellPos Start() const { return definition_.start; }

private:
    // Save a clean copy so a restart can put everything back
    LevelDefinition definition_;
    Grid tiles_{};
    std::vector<CellPos> cargoStations_;
    std::vector<bool> cargoClaimed_;
    std::vector<bool> cargoDelivered_;
};
}
