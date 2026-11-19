#pragma once

#include "GameTypes.h"

namespace tri {
// Keeps the map tiles and cargo stations in one place
class Level {
public:
    // The board is a fixed grid of game tiles
    using Grid = std::array<std::array<Tile, GridWidth>, GridHeight>;
    using PortalLink = std::pair<CellPos, CellPos>;

    explicit Level(LevelDefinition definition);

    // Restore the map before a new run
    void Reset();
    bool Update(float deltaTime);

    // Basic checks used by movement and pathfinding
    bool IsInside(int x, int y) const;
    bool IsPathable(int x, int y) const;
    int Distance(CellPos a, CellPos b) const;
    Tile At(int x, int y) const;

    // Let the game and renderer read the map data
    const Grid& Tiles() const { return tiles_; }
    const LevelDefinition& Definition() const { return definition_; }
    const std::vector<CellPos>& CargoStations() const { return cargoStations_; }
    const std::vector<CellPos>& BridgeCells() const { return bridgeCells_; }
    const std::vector<PortalLink>& PortalLinks() const { return portalLinks_; }
    CellPos Start() const { return definition_.start; }
    bool HasBridgeEvent() const { return !bridgeCells_.empty() && definition_.bridgeCollapseSeconds > 0.0f; }
    bool BridgeCollapsed() const { return bridgeCollapsed_; }
    float BridgeCollapseEffectTime() const { return bridgeCollapseEffectTime_; }
    float BridgeCollapseRemaining() const;
    CellPos PortalExit(CellPos cell) const;

private:
    // Keep the original map so Reset can restore it
    LevelDefinition definition_;
    Grid tiles_{};
    std::vector<CellPos> cargoStations_;
    std::vector<CellPos> bridgeCells_;
    std::vector<PortalLink> portalLinks_;
    float eventTime_ = 0.0f;
    float bridgeCollapseEffectTime_ = 0.0f;
    bool bridgeCollapsed_ = false;
};
}
