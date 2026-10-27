#pragma once

#include "Level.h"
#include <vector>

namespace tri {
// Finds a cheap route around walls, with beacons nudging the way
class Pathfinder {
public:
    // Give back the cells in order from start to target
    static std::vector<CellPos> FindPath(
        const Level& level,
        CellPos from,
        CellPos to,
        const std::vector<Beacon>& beacons);
};
}
