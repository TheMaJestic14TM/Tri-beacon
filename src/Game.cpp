#include "Game.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>

#ifndef TRI_BEACON_LEVEL_DIR
#define TRI_BEACON_LEVEL_DIR "levels"
#endif

namespace tri {
using namespace std;
namespace {
const Color GreenColor = {74, 218, 143, 255};
const Color BlueColor = {75, 173, 255, 255};
const Color RedColor = {255, 100, 103, 255};

// ============================================================
// Beacon setup
// Give each beacon its color and type when it is placed
Beacon MakeBeacon(int type, CellPos cell) {
    Beacon beacon;
    beacon.cell = cell;
    beacon.type = type;

    if (type == 0) {
        beacon.color = GreenColor;
    } else if (type == 1) {
        beacon.color = BlueColor;
    } else {
        beacon.color = RedColor;
    }

    return beacon;
}

bool IsInsideRectangle(Vector2 point, int x, int y, int width, int height) {
    return point.x >= x && point.x < x + width &&
        point.y >= y && point.y < y + height;
}
}

// Load the first valid map from the levels folder
Game::Game() {
    vector<LevelDefinition> levels = LoadLevelCatalog(TRI_BEACON_LEVEL_DIR);

    if (levels.empty()) {
        running_ = false;
        notice_.message = "No valid level found";
        return;
    }

    level_ = make_unique<Level>(levels[0]);
    Reset();
}

// Give the map and the current run a clean restart
void Game::Reset() {
    if (!level_) {
        return;
    }

    level_->Reset();
    pawns_.clear();
    beacons_.clear();
    stats_ = {};
    notice_ = {};
    selectedBeaconType_ = 0;
    maxBeaconsUsed_ = 0;
    started_ = false;
    paused_ = false;
    running_ = true;
    won_ = false;
}

// Show a short message above the board
void Game::SetNotice(const string& message) {
    notice_.message = message;
    notice_.timeLeft = 3.0f;
}

// ============================================================
// Beacon count and level limits
// Read how many of each type are placed and how many are allowed
int Game::BeaconCount(int type) const {
    int count = 0;
    for (const Beacon& beacon : beacons_) {
        if (beacon.type == type) {
            count++;
        }
    }
    return count;
}

int Game::BeaconLimit(int type) const {
    if (!level_) {
        return 0;
    }

    const LevelDefinition& rules = level_->Definition();
    if (type == 0) return rules.greenBeaconLimit;
    if (type == 1) return rules.blueBeaconLimit;
    return rules.redBeaconLimit;
}

int Game::StationCount() const {
    if (!level_) {
        return 0;
    }
    return static_cast<int>(level_->CargoStations().size());
}

bool Game::CanStart() const {
    return running_ && !started_ && level_ && BeaconCount(0) > 0;
}

bool Game::CanRelease() const {
    return running_ && started_ && !paused_;
}

// Put the next minion right on the start tile
void Game::SpawnPawn() {
    CellPos start = level_->Start();
    Vector2 position = {start.x + 0.5f, start.y + 0.5f};

    Pawn pawn;
    pawn.position = position;
    pawn.cell = start;
    pawns_.push_back(pawn);
    stats_.deployed++;
}

void Game::StartRun() {
    if (!CanStart()) {
        if (BeaconCount(0) == 0) {
            SetNotice("Place a green beacon before starting");
        }
        return;
    }

    started_ = true;
    maxBeaconsUsed_ = static_cast<int>(beacons_.size());
    SetNotice("Run started · Release a minion when ready");
}

void Game::ReleasePawn() {
    if (!CanRelease() || !level_) {
        return;
    }

    SpawnPawn();
    SetNotice("Minion released · You can still change the beacons");
}

void Game::GiveUp() {
    if (!running_ || !started_) {
        return;
    }

    running_ = false;
    SetNotice("Run ended · Press R to try again");
}

// ============================================================
// Cargo only becomes a target inside its local 3x3 area
int Game::FindNearbyCargoStation(CellPos from) const {
    int bestIndex = -1;
    int bestDistance = 1000000;
    const vector<CellPos>& stations = level_->CargoStations();

    for (int i = 0; i < static_cast<int>(stations.size()); i++) {
        if (!level_->IsCargoAvailable(i)) {
            continue;
        }

        int distanceX = abs(from.x - stations[i].x);
        int distanceY = abs(from.y - stations[i].y);
        if (distanceX > CargoAreaRadius || distanceY > CargoAreaRadius) {
            continue;
        }

        int distance = level_->Distance(from, stations[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

// ============================================================  i love potatos
// 
// Choose the next green waypoint the minion has not visited
int Game::FindNextGreenBeacon(CellPos from, const vector<CellPos>& visited) const {
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (int i = 0; i < static_cast<int>(beacons_.size()); i++) {
        if (beacons_[i].type != 0) {
            continue;
        }

        bool alreadyVisited = false;
        for (CellPos cell : visited) {
            if (cell == beacons_[i].cell) {
                alreadyVisited = true;
                break;
            }
        }

        if (alreadyVisited) {
            continue;
        }

        int distance = level_->Distance(from, beacons_[i].cell);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

// Move one minion and handle anything it reaches
void Game::UpdatePawn(Pawn& pawn, float deltaTime) {
    pawn.cell = {static_cast<int>(pawn.position.x), static_cast<int>(pawn.position.y)};

    // Step onto a hazard and this minion is out
    if (level_->At(pawn.cell.x, pawn.cell.y) == Tile::Hazard) {
        if (pawn.targetStationIndex >= 0) {
            level_->ReleaseCargoStation(pawn.targetStationIndex);
        }
        for (int stationIndex : pawn.cargoStationIndexes) {
            level_->ReleaseCargoStation(stationIndex);
        }

        pawn.alive = false;
        pawn.cargoCount = 0;
        pawn.targetStationIndex = -1;
        pawn.cargoStationIndexes.clear();
        stats_.lost++;
        return;
    }

    // ============================================================
    // Cargo only becomes a goal when the minion is nearby
    if (pawn.targetStationIndex < 0) {
        int stationIndex = FindNearbyCargoStation(pawn.cell);
        if (stationIndex >= 0 && level_->ClaimCargoStation(stationIndex)) {
            pawn.targetStationIndex = stationIndex;
            pawn.hasGreenWaypoint = false;
        }
    }

    // ============================================================
    // A minion picks up each cargo it finds along its beacon route
    CellPos target = level_->Start();

    if (pawn.targetStationIndex >= 0) {
        const vector<CellPos>& stations = level_->CargoStations();
        CellPos station = stations[pawn.targetStationIndex];
        target = station;

        if (pawn.cell == station) {
            pawn.cargoStationIndexes.push_back(pawn.targetStationIndex);
            pawn.targetStationIndex = -1;
            pawn.cargoCount++;
        }
    }

    if (pawn.targetStationIndex < 0) {
        // ============================================================
        // Green beacons lead minions through the map
        if (pawn.hasGreenWaypoint) {
            bool waypointStillThere = false;
            for (const Beacon& beacon : beacons_) {
                if (beacon.type == 0 && beacon.cell == pawn.greenWaypoint) {
                    waypointStillThere = true;
                    break;
                }
            }

            if (!waypointStillThere) {
                pawn.hasGreenWaypoint = false;
            }
        }

        if (pawn.hasGreenWaypoint && pawn.cell == pawn.greenWaypoint) {
            pawn.visitedGreenBeacons.push_back(pawn.greenWaypoint);
            pawn.hasGreenWaypoint = false;
        }

        if (!pawn.hasGreenWaypoint) {
            int greenIndex = FindNextGreenBeacon(pawn.cell, pawn.visitedGreenBeacons);

            if (greenIndex >= 0) {
                pawn.greenWaypoint = beacons_[greenIndex].cell;
                pawn.hasGreenWaypoint = true;
                target = pawn.greenWaypoint;
            } else {
                target = level_->Start();
            }
        } else {
            target = pawn.greenWaypoint;
        }
    }

    if (pawn.targetStationIndex < 0 && !pawn.hasGreenWaypoint &&
        target == level_->Start() && pawn.cell == level_->Start()) {
        if (pawn.cargoCount > 0) {
            for (int stationIndex : pawn.cargoStationIndexes) {
                level_->DeliverCargoStation(stationIndex);
                stats_.cargoDelivered++;
            }

            stats_.returned++;
        }

        pawn.cargoStationIndexes.clear();
        pawn.cargoCount = 0;
        pawn.alive = false;
        return;
    }

    vector<CellPos> route = Pathfinder::FindPath(*level_, pawn.cell, target, beacons_);
    if (route.size() < 2) {
        return;
    }

    CellPos nextCell = route[1];
    float speed = 2.6f;
    if (level_->At(nextCell.x, nextCell.y) == Tile::Water) {
        speed = 1.7f;
    }

    Vector2 targetPosition = {nextCell.x + 0.5f, nextCell.y + 0.5f};
    float moveX = targetPosition.x - pawn.position.x;
    float moveY = targetPosition.y - pawn.position.y;
    float distance = sqrt(moveX * moveX + moveY * moveY);
    float moveDistance = speed * deltaTime;

    // Land right in the middle when we reach the next tile
    if (distance <= moveDistance) {
        pawn.position = targetPosition;
        pawn.cell = nextCell;
        return;
    }

    // Otherwise keep moving toward that tile
    pawn.position.x += moveX / distance * moveDistance;
    pawn.position.y += moveY / distance * moveDistance;
}

// ============================================================
// Run progress and win check
// Update the level and check if all cargo is home
void Game::Update(float deltaTime) {
    if (!running_ || paused_ || !started_ || !level_) {
        return;
    }

    notice_.timeLeft = max(0.0f, notice_.timeLeft - deltaTime);
    if (stats_.deployed == 0) {
        return;
    }

    stats_.elapsedSeconds += deltaTime;

    for (Pawn& pawn : pawns_) {
        if (pawn.alive) {
            UpdatePawn(pawn, deltaTime);
        }
    }

    if (stats_.cargoDelivered >= StationCount()) {
        won_ = true;
        running_ = false;

        float time = max(0.1f, stats_.elapsedSeconds);
        int beaconCount = max(1, maxBeaconsUsed_);
        stats_.score = static_cast<int>(BaseScore / (time * beaconCount));
        SetNotice("Level complete · Press R to play again");
    }
}

// ============================================================
// Beacon placement
// Check the selected type and level limit before adding a beacon
void Game::PlaceBeacon(CellPos cell) {
    if (!level_ || !level_->IsPathable(cell.x, cell.y)) {
        return;
    }
    if (level_->At(cell.x, cell.y) == Tile::Hazard) {
        SetNotice("Beacons need a safe tile");
        return;
    }

    for (const Beacon& beacon : beacons_) {
        if (beacon.cell == cell && beacon.type == selectedBeaconType_) {
            SetNotice("That beacon is already there");
            return;
        }
    }

    if (BeaconCount(selectedBeaconType_) >= BeaconLimit(selectedBeaconType_)) {
        SetNotice("No more of that beacon type for this level");
        return;
    }

    beacons_.push_back(MakeBeacon(selectedBeaconType_, cell));
    if (started_) {
        maxBeaconsUsed_ = max(maxBeaconsUsed_, static_cast<int>(beacons_.size()));
    }

    // A new green beacon gives active minions a fresh waypoint
    if (selectedBeaconType_ == 0) {
        for (Pawn& pawn : pawns_) {
            pawn.hasGreenWaypoint = false;
            pawn.visitedGreenBeacons.clear();
        }
    }
}

// ============================================================
// Beacon removal
// Removing a green beacon makes minions choose a new waypoint
void Game::RemoveBeacon(CellPos cell) {
    for (int i = static_cast<int>(beacons_.size()) - 1; i >= 0; i--) {
        if (beacons_[i].cell == cell) {
            int removedType = beacons_[i].type;
            beacons_.erase(beacons_.begin() + i);
            SetNotice("Beacon removed");

            if (removedType == 0) {
                for (Pawn& pawn : pawns_) {
                    pawn.hasGreenWaypoint = false;
                    pawn.visitedGreenBeacons.clear();
                }
            }
            return;
        }
    }
}

// Handle clicks on the buttons beside the map
void Game::HandlePanelClick(Vector2 mouse) {
    if (IsInsideRectangle(mouse, ControlButtonX, StartButtonY, ControlButtonWidth, ControlButtonHeight)) {
        StartRun();
    } else if (IsInsideRectangle(mouse, ControlButtonX, ReleaseButtonY, ControlButtonWidth, ControlButtonHeight)) {
        ReleasePawn();
    } else if (IsInsideRectangle(mouse, ControlButtonX, GiveUpButtonY, ControlButtonWidth, ControlButtonHeight)) {
        GiveUp();
    }
}

// Check the keys and clicks the player just made
void Game::HandleInput() {
    if (IsKeyPressed(KEY_R)) {
        Reset();
        return;
    }

    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    if (!running_) {
        return;
    }

    if (IsKeyPressed(KEY_ONE)) selectedBeaconType_ = 0;
    if (IsKeyPressed(KEY_TWO)) selectedBeaconType_ = 1;
    if (IsKeyPressed(KEY_THREE)) selectedBeaconType_ = 2;

    if (IsKeyPressed(KEY_ENTER)) {
        StartRun();
    }

    if (IsKeyPressed(KEY_N)) {
        ReleasePawn();
    }

    if (IsKeyPressed(KEY_SPACE) && started_) {
        paused_ = !paused_;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        GiveUp();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = ScreenToGamePosition(GetMousePosition());
        if (IsInsideRectangle(mouse, ControlPanelX, BoardTop, ControlPanelWidth, GridHeight * CellSize)) {
            HandlePanelClick(mouse);
            return;
        }

        int boardRight = BoardLeft + GridWidth * CellSize;
        int boardBottom = BoardTop + GridHeight * CellSize;
        bool insideBoard = mouse.x >= BoardLeft && mouse.x < boardRight;
        insideBoard = insideBoard && mouse.y >= BoardTop && mouse.y < boardBottom;

        if (insideBoard) {
            CellPos cell;
            cell.x = static_cast<int>((mouse.x - BoardLeft) / CellSize);
            cell.y = static_cast<int>((mouse.y - BoardTop) / CellSize);
            PlaceBeacon(cell);
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mouse = ScreenToGamePosition(GetMousePosition());
        int boardRight = BoardLeft + GridWidth * CellSize;
        int boardBottom = BoardTop + GridHeight * CellSize;
        bool insideBoard = mouse.x >= BoardLeft && mouse.x < boardRight;
        insideBoard = insideBoard && mouse.y >= BoardTop && mouse.y < boardBottom;

        if (insideBoard) {
            CellPos cell;
            cell.x = static_cast<int>((mouse.x - BoardLeft) / CellSize);
            cell.y = static_cast<int>((mouse.y - BoardTop) / CellSize);
            RemoveBeacon(cell);
        }
    }
}
}
