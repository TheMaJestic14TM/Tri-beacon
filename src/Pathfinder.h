#pragma once

#include "Level.h"
#include <vector>

namespace tri {
// Finds a low-cost route while beacons affect the path
class Pathfinder {
public:
    // Return the route cells from start to target
    static std::vector<CellPos> FindPath(
        const Level& level,
        CellPos from,
        CellPos to,
        const std::vector<Beacon>& beacons);
};
}
