#pragma once

#include "Level.h"
#include <vector>

namespace tri {
/**
 * @brief Finds a low-cost route across the current map with A star
 * @details The search checks four neighboring cells at a time. Water adds
 * movement cost, while blue and red beacons lower or raise the cost nearby.
 */
class Pathfinder {
public:
    /**
     * @brief Find a route from one cell to another
     * @param level Map used for terrain and walkability checks
     * @param from Starting cell
     * @param to Target cell
     * @param beacons Beacons that change nearby movement costs
     * @return Ordered route cells, or an empty list when no route exists
     * @note The minion can only move through four-way connected cells.
     */
    static std::vector<CellPos> FindPath(
        const Level& level,
        CellPos from,
        CellPos to,
        const std::vector<Beacon>& beacons);
};
}
