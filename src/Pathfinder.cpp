#include "Pathfinder.h"
#include <algorithm>
#include <queue>

namespace tri {
using namespace std;

// Find the cheapest way from one cell to another
vector<CellPos> Pathfinder::FindPath(
    const Level& level,
    CellPos from,
    CellPos to,
    const vector<Beacon>& beacons) {

    // The queue keeps track of cells we might check next
    struct Node {
        float score;
        CellPos cell;

        // Put the cheapest looking cell at the front
        bool operator<(const Node& other) const {
            return score > other.score;
        }
    };

    array<array<float, GridWidth>, GridHeight> costs;
    array<array<CellPos, GridWidth>, GridHeight> previous{};
    array<array<bool, GridWidth>, GridHeight> visited{};

    for (auto& row : costs) {
        row.fill(1000000.0f);
    }

    if (!level.IsPathable(from.x, from.y) || !level.IsPathable(to.x, to.y)) {
        return {};
    }

    priority_queue<Node> open;
    costs[from.y][from.x] = 0.0f;

    // This quick distance estimate helps A star aim toward the goal
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

        // Take a look at the four neighbors around this cell
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

            // Blue makes nearby tiles tempting and red makes them costly
            for (const Beacon& beacon : beacons) {
                int xDistance = abs(next.x - beacon.cell.x);
                int yDistance = abs(next.y - beacon.cell.y);
                bool inBeaconArea = xDistance <= BeaconAreaRadius && yDistance <= BeaconAreaRadius;

                if (!inBeaconArea) {
                    continue;
                }

                if (beacon.type == 1) {
                    stepCost -= 0.12f;
                }
                if (beacon.type == 2) {
                    stepCost += 0.6f;
                }
            }

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

    // Start at the goal and follow our breadcrumbs back to the start
    vector<CellPos> route;
    CellPos current = to;
    route.push_back(current);

    while (current != from) {
        current = previous[current.y][current.x];
        route.push_back(current);

        // Bail out if something went weird and the route loops too long
        if (route.size() > GridWidth * GridHeight) {
            return {};
        }
    }

    reverse(route.begin(), route.end());
    return route;
}
}
