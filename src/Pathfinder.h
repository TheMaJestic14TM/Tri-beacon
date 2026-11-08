#pragma once

#include "Level.h"
#include <vector>

namespace tri {
// Finds a cheap route across the current map
class Pathfinder {
public:
    // Return each grid cell from the start to the target
    static std::vector<CellPos> FindPath(
        const Level& level,
        CellPos from,
        CellPos to,
        const std::vector<Beacon>& beacons);
};
}
