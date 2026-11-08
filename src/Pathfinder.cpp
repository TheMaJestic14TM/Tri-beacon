#include "Pathfinder.h"
#include <algorithm>
#include <cstdlib>
#include <queue>

namespace tri {
using namespace std;

// ==================== A STAR PATHFINDING ====================
// Find the cheapest route from one cell to another
vector<CellPos> Pathfinder::FindPath(
    const Level& level,
    CellPos from,
    CellPos to,
    const vector<Beacon>& beacons) {

    // Each node stores the next cell and its route score
    struct Node {
        float score;
        CellPos cell;

        bool operator<(const Node& other) const {
            return score > other.score;
        }
    };

    // Save route costs and breadcrumbs for the final path
    array<array<float, GridWidth>, GridHeight> costs;
    array<array<CellPos, GridWidth>, GridHeight> previous{};
    array<array<bool, GridWidth>, GridHeight> visited{};

    for (auto& row : costs) {
        row.fill(1000000.0f);
    }

    if (!level.IsPathable(from.x, from.y) || !level.IsPathable(to.x, to.y)) {
        return {};
    }

    // Start searching from the minion's current cell
    priority_queue<Node> open;
    costs[from.y][from.x] = 0.0f;

    float firstEstimate = level.Distance(from, to) * 0.25f;
    open.push({firstEstimate, from});

    const int moveX[4] = {1, -1, 0, 0};
    const int moveY[4] = {0, 0, 1, -1};

    while (!open.empty()) {
        CellPos current = open.top().cell;
        open.pop();

        if (visited[current.y][current.x]) {
            continue;
        }
        visited[current.y][current.x] = true;

        if (current == to) {
            break;
        }

        // Check the four cells next to this one
        for (int direction = 0; direction < 4; direction++) {
            CellPos next;
            next.x = current.x + moveX[direction];
            next.y = current.y + moveY[direction];

            if (!level.IsPathable(next.x, next.y) || visited[next.y][next.x]) {
                continue;
            }

            float stepCost = 1.0f;
            if (level.At(next.x, next.y) == Tile::Water) {
                stepCost += 1.5f;
            }

            // ==================== BEACON COSTS ====================
            // Beacons change the cost of nearby cells
            for (const Beacon& beacon : beacons) {
                int xDistance = abs(next.x - beacon.cell.x);
                int yDistance = abs(next.y - beacon.cell.y);
                bool inBeaconArea = xDistance <= BeaconAreaRadius && yDistance <= BeaconAreaRadius;

                if (!inBeaconArea) {
                    continue;
                }

                // ==================== BLUE BEACON ====================
                // Blue beacons make this route cheaper
                if (beacon.type == BlueBeaconType) {
                    stepCost -= 0.5f;
                }

                // ==================== RED BEACON ====================
                // Red beacons make this route more costly
                if (beacon.type == RedBeaconType) {
                    stepCost += 2.5f;
                }
            }

            stepCost = max(0.25f, stepCost);

            float newCost = costs[current.y][current.x] + stepCost;
            if (newCost >= costs[next.y][next.x]) {
                continue;
            }

            costs[next.y][next.x] = newCost;
            previous[next.y][next.x] = current;

            float estimate = level.Distance(next, to) * 0.25f;
            open.push({newCost + estimate, next});
        }
    }

    if (!visited[to.y][to.x]) {
        return {};
    }

    // Follow the breadcrumbs back to the start
    vector<CellPos> route;
    CellPos current = to;
    route.push_back(current);

    while (current != from) {
        current = previous[current.y][current.x];
        route.push_back(current);

        if (route.size() > GridWidth * GridHeight) {
            return {};
        }
    }

    reverse(route.begin(), route.end());
    return route;
}
}
