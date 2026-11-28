#pragma once

#include "GameTypes.h"

namespace tri {
/** @brief Stores one level's map and updates its world events */
class Level {
public:
    /** @brief A fixed grid of terrain tiles */
    using Grid = std::array<std::array<Tile, GridWidth>, GridHeight>;
    /** @brief A one-way link from a portal entrance to its exit */
    using PortalLink = std::pair<CellPos, CellPos>;

    /** @brief Create a level from its loaded map data */
    explicit Level(LevelDefinition definition);

    /** @brief Restore the original map before a new run */
    void Reset();
    /**
     * @brief Update level events and report whether the bridge collapsed
     * @param deltaTime Time since the last update in seconds
     */
    bool Update(float deltaTime);

    /**
     * @brief Check whether a cell is inside the map
     * @param x Horizontal cell coordinate
     * @param y Vertical cell coordinate
     */
    bool IsInside(int x, int y) const;
    /**
     * @brief Check whether a minion can move through a cell
     * @param x Horizontal cell coordinate
     * @param y Vertical cell coordinate
     */
    bool IsPathable(int x, int y) const;
    /**
     * @brief Get the Manhattan distance between two cells
     * @param a First cell
     * @param b Second cell
     */
    int Distance(CellPos a, CellPos b) const;
    /**
     * @brief Get the terrain at a cell, or a wall outside the map
     * @param x Horizontal cell coordinate
     * @param y Vertical cell coordinate
     */
    Tile At(int x, int y) const;

    /** @brief Get the current terrain grid */
    const Grid& Tiles() const { return tiles_; }
    /** @brief Get the original level definition */
    const LevelDefinition& Definition() const { return definition_; }
    /** @brief Get all cargo station positions */
    const std::vector<CellPos>& CargoStations() const { return cargoStations_; }
    /** @brief Get all bridge positions */
    const std::vector<CellPos>& BridgeCells() const { return bridgeCells_; }
    /** @brief Get all one-way portal links */
    const std::vector<PortalLink>& PortalLinks() const { return portalLinks_; }
    /** @brief Get the minion start cell */
    CellPos Start() const { return definition_.start; }
    /** @brief Check whether this level has a timed bridge event */
    bool HasBridgeEvent() const { return !bridgeCells_.empty() && definition_.bridgeCollapseSeconds > 0.0f; }
    /** @brief Check whether the bridge has collapsed */
    bool BridgeCollapsed() const { return bridgeCollapsed_; }
    /** @brief Get the remaining time for the bridge collapse effect */
    float BridgeCollapseEffectTime() const { return bridgeCollapseEffectTime_; }
    /** @brief Get the time left before the bridge collapses */
    float BridgeCollapseRemaining() const;
    /**
     * @brief Get the exit paired with a portal entrance
     * @param cell Portal entrance cell
     * @return Exit cell, or an invalid cell if there is no match
     */
    CellPos PortalExit(CellPos cell) const;

private:
    /** @brief Original level data used to reset the map */
    LevelDefinition definition_;
    /** @brief Terrain grid after any level events */
    Grid tiles_{};
    /** @brief Cargo station positions found in the map */
    std::vector<CellPos> cargoStations_;
    /** @brief Bridge positions found in the map */
    std::vector<CellPos> bridgeCells_;
    /** @brief Entrance and exit pairs for one-way portals */
    std::vector<PortalLink> portalLinks_;
    /** @brief Time passed since the level event started */
    float eventTime_ = 0.0f;
    /** @brief Time left for the bridge collapse effect */
    float bridgeCollapseEffectTime_ = 0.0f;
    /** @brief True after the bridge has collapsed */
    bool bridgeCollapsed_ = false;
};
}
