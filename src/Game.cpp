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

Beacon MakeBeacon(int type, CellPos cell) {
    Beacon beacon;
    beacon.cell = cell;
    beacon.type = type;

    if (type == 0) {
        beacon.color = GreenColor;
        beacon.label = "GREEN";
    } else if (type == 1) {
        beacon.color = BlueColor;
        beacon.label = "BLUE";
    } else {
        beacon.color = RedColor;
        beacon.label = "RED";
    }

    return beacon;
}

bool IsInsideRectangle(Vector2 point, int x, int y, int width, int height) {
    return point.x >= x && point.x < x + width &&
        point.y >= y && point.y < y + height;
}
}

// Grab the first map in the levels folder that actually loads
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
    givenUp_ = false;
}

// Pop a quick message above the board
void Game::SetNotice(const string& message) {
    notice_.message = message;
    notice_.timeLeft = 3.0f;
}

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
    pawn.visitedStations.resize(level_->CargoStations().size(), false);
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
    givenUp_ = true;
    SetNotice("Run ended · Press R to try again");
}

// Pick the closest cargo station this minion has not visited yet
int Game::FindNextStation(const Pawn& pawn) const {
    vector<CellPos> stations = level_->CargoStations();
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (int i = 0; i < static_cast<int>(stations.size()); i++) {
        if (pawn.visitedStations[i]) {
            continue;
        }

        int distance = level_->Distance(pawn.cell, stations[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

// Find the closest green beacon to this minion
int Game::FindNearestGreenBeacon(CellPos from) const {
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (int i = 0; i < static_cast<int>(beacons_.size()); i++) {
        if (beacons_[i].type != 0) {
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

// Move one minion along and see what it runs into
void Game::UpdatePawn(Pawn& pawn, float deltaTime) {
    pawn.cell = {static_cast<int>(pawn.position.x), static_cast<int>(pawn.position.y)};

    // Step onto a hazard and this minion is out
    if (level_->At(pawn.cell.x, pawn.cell.y) == Tile::Hazard) {
        pawn.alive = false;
        pawn.cargoCount = 0;
        stats_.lost++;
        return;
    }

    // A station keeps its cargo for the next minion too
    vector<CellPos> stations = level_->CargoStations();
    for (int i = 0; i < static_cast<int>(stations.size()); i++) {
        if (pawn.visitedStations[i]) {
            continue;
        }

        if (level_->IsInsideCargoArea(pawn.cell, stations[i])) {
            pawn.visitedStations[i] = true;
            pawn.cargoCount++;
            pawn.hasGreenWaypoint = false;
            pawn.passedGreenWaypoint = false;
        }
    }

    int nextStation = FindNextStation(pawn);
    bool hasStation = nextStation >= 0;
    CellPos target = level_->Start();

    if (hasStation) {
        target = stations[nextStation];
    }

    // Visit the nearest green beacon before heading to the next task
    if (!pawn.passedGreenWaypoint) {
        if (pawn.hasGreenWaypoint) {
            bool waypointStillThere = false;
            for (const Beacon& beacon : beacons_) {
                if (beacon.type == 0 && beacon.cell == pawn.greenWaypoint) {
                    waypointStillThere = true;
                }
            }

            if (!waypointStillThere) {
                pawn.hasGreenWaypoint = false;
            }
        }

        if (!pawn.hasGreenWaypoint) {
            int greenIndex = FindNearestGreenBeacon(pawn.cell);
            if (greenIndex >= 0) {
                pawn.greenWaypoint = beacons_[greenIndex].cell;
                pawn.hasGreenWaypoint = true;
            } else {
                pawn.passedGreenWaypoint = true;
            }
        }

        if (pawn.hasGreenWaypoint) {
            if (pawn.cell == pawn.greenWaypoint) {
                pawn.hasGreenWaypoint = false;
                pawn.passedGreenWaypoint = true;
            } else {
                target = pawn.greenWaypoint;
            }
        }
    }

    // Once every station is visited, head back to the start
    if (!hasStation && pawn.cell == level_->Start() && pawn.cargoCount > 0) {
        pawn.alive = false;
        stats_.returned++;
        stats_.cargoDelivered += pawn.cargoCount;
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

// Keep the level moving once the player starts the run
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

    const LevelDefinition& rules = level_->Definition();
    if (stats_.cargoDelivered >= rules.cargoGoal) {
        won_ = true;
        running_ = false;

        float time = max(0.1f, stats_.elapsedSeconds);
        int beaconCount = max(1, maxBeaconsUsed_);
        stats_.score = static_cast<int>(BaseScore / (time * beaconCount));
        SetNotice("Level complete · Press R to play again");
    }
}

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
            pawn.passedGreenWaypoint = false;
        }
    }
}

void Game::RemoveBeacon(CellPos cell) {
    for (int i = static_cast<int>(beacons_.size()) - 1; i >= 0; i--) {
        if (beacons_[i].cell == cell) {
            int removedType = beacons_[i].type;
            beacons_.erase(beacons_.begin() + i);
            SetNotice("Beacon removed");

            if (removedType == 0) {
                for (Pawn& pawn : pawns_) {
                    pawn.hasGreenWaypoint = false;
                    pawn.passedGreenWaypoint = false;
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
        Vector2 mouse = GetMousePosition();
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
        Vector2 mouse = GetMousePosition();
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
