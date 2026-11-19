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
// Beacon colors stay together so they are easy to tweak
const Color GreenColor = {74, 218, 143, 255};
const Color BlueColor = {75, 173, 255, 255};
const Color RedColor = {255, 100, 103, 255};

Beacon MakeBeacon(int type, CellPos cell) {
    Beacon beacon;
    beacon.cell = cell;
    beacon.type = type;

    if (type == GreenBeaconType) {
        beacon.color = GreenColor;
    } else if (type == BlueBeaconType) {
        beacon.color = BlueColor;
    } else {
        beacon.color = RedColor;
    }

    return beacon;
}

// Green beacons cost more because they guide the route
int BeaconCost(int type) {
    return type == GreenBeaconType ? 2 : 1;
}

// Used for clicks on the side panel buttons
bool IsInsideRectangle(Vector2 point, int x, int y, int width, int height) {
    return point.x >= x && point.x < x + width &&
        point.y >= y && point.y < y + height;
}

// A cell only counts as reached when the minion gets to its center
bool IsAtCellCenter(Vector2 position, CellPos cell) {
    float centerX = cell.x + 0.5f;
    float centerY = cell.y + 0.5f;
    return abs(position.x - centerX) < 0.02f && abs(position.y - centerY) < 0.02f;
}
}

// ==================== GAME SETUP ====================
Game::Game() {
    levelCatalog_ = LoadLevelCatalog(TRI_BEACON_LEVEL_DIR);

    if (levelCatalog_.empty()) {
        running_ = false;
        notice_.message = "No valid level found";
        return;
    }

    level_ = make_unique<Level>(levelCatalog_[currentLevelIndex_]);
    Reset();
}

// Clear the old run and set the map back to its start
void Game::Reset() {
    if (!level_) {
        return;
    }

    level_->Reset();
    minions_.clear();
    beacons_.clear();
    stats_ = {};
    notice_ = {};
    selectedBeaconType_ = GreenBeaconType;
    runBeaconCost_ = 0;
    spawnMinions_ = false;
    started_ = false;
    paused_ = false;
    running_ = true;
    won_ = false;
    bridgeWarningShown_ = false;
}

// Show a quick note when the player needs one
void Game::SetNotice(const string& message) {
    notice_.message = message;
    notice_.timeLeft = 3.0f;
}

// Switch maps with the temporary level navigation keys
void Game::ChangeLevel(int direction) {
    if (levelCatalog_.size() < 2) {
        return;
    }

    int levelCount = static_cast<int>(levelCatalog_.size());
    currentLevelIndex_ = (currentLevelIndex_ + direction + levelCount) % levelCount;
    level_ = make_unique<Level>(levelCatalog_[currentLevelIndex_]);
    Reset();
}

// ==================== BEACON INFO ====================
// Count how many beacons of one color are on the map
int Game::BeaconCount(int type) const {
    int count = 0;
    for (const Beacon& beacon : beacons_) {
        if (beacon.type == type) {
            count++;
        }
    }
    return count;
}

// Cargo stations are placed in the Tiled map
int Game::StationCount() const {
    if (!level_) {
        return 0;
    }
    return static_cast<int>(level_->CargoStations().size());
}

// A run needs at least one green beacon to begin
bool Game::CanStart() const {
    return running_ && !started_ && level_ && BeaconCount(GreenBeaconType) > 0;
}

// ==================== MINION SPAWNING ====================
// Spawns minions automatically
void Game::SpawnMinion() {
    CellPos start = level_->Start();
    Vector2 position = {start.x + 0.5f, start.y + 0.5f};

    Minion minion;
    minion.position = position;
    minion.cell = start;
    minions_.push_back(minion);
    stats_.spawned++;
}

void Game::UpdateSpawner(float deltaTime) {
    spawnTimer_ += deltaTime;
    while (spawnTimer_ >= 0.25f) {
        spawnTimer_ -= 0.25f;
        SpawnMinion();
    }
}


// ==================== RUN CONTROLS ====================
void Game::StartRun() {
    if (!CanStart()) {
        if (BeaconCount(GreenBeaconType) == 0) {
            SetNotice("Place a green beacon before starting");
        }
        return;
    }

    started_ = true;
    spawnMinions_ = true;
    // Only the beacons still on the map count from setup
    runBeaconCost_ = 0;
    for (const Beacon& beacon : beacons_) {
        runBeaconCost_ += BeaconCost(beacon.type);
    }
    SetNotice("Run started");
}

// The player can give up at any time during a run
void Game::GiveUp() {
    if (!running_ || !started_) {
        return;
    }

    running_ = false;
    SetNotice("Run ended · Press R to try again");
}

// ==================== CARGO STATIONS ====================
// Minions only notice a station when they enter its 5x5 area
int Game::FindStationInRange(const Minion& minion) const {
    int bestIndex = -1;
    int bestDistance = GridWidth + GridHeight;
    const vector<CellPos>& stations = level_->CargoStations();
    CellPos from = minion.cell;

    for (int i = 0; i < static_cast<int>(stations.size()); i++) {
        if (HasVisitedStation(minion, i)) {
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

// A minion can only pick up each station once per trip
bool Game::HasVisitedStation(const Minion& minion, int stationIndex) const {
    return find(minion.stationsVisited.begin(), minion.stationsVisited.end(), stationIndex)
        != minion.stationsVisited.end();
}

// A full trip means every station has been visited
bool Game::VisitedEveryStation(const Minion& minion) const {
    return StationCount() > 0 && static_cast<int>(minion.stationsVisited.size()) == StationCount();
}

// ==================== GREEN BEACON ROUTE ====================
// Find the nearest green beacon this minion has not visited yet
int Game::FindNextGreenWaypoint(CellPos from, const vector<CellPos>& visited) const {
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (int i = 0; i < static_cast<int>(beacons_.size()); i++) {
        if (beacons_[i].type != GreenBeaconType) {
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

// ==================== MINION MOVEMENT ====================
// Follow the beacons, collect nearby cargo and head home
void Game::UpdateMinion(Minion& minion, float deltaTime) {
    minion.cell = {static_cast<int>(minion.position.x), static_cast<int>(minion.position.y)};

    if (level_->At(minion.cell.x, minion.cell.y) == Tile::Hazard) {
        minion.alive = false;
        minion.loadsCarried = 0;
        minion.stationTarget = -1;
        stats_.lost++;
        return;
    }

    // Reaching a green waypoint counts even when it is on a teleporter
    if (minion.hasGreenWaypoint && minion.cell == minion.greenWaypoint &&
        IsAtCellCenter(minion.position, minion.greenWaypoint)) {
        minion.visitedGreenBeacons.push_back(minion.greenWaypoint);
        minion.hasGreenWaypoint = false;
    }

    // Nearby cargo keeps priority when a minion reaches a portal
    if (minion.stationTarget < 0) {
        int stationIndex = FindStationInRange(minion);
        if (stationIndex >= 0) {
            minion.stationTarget = stationIndex;
            minion.hasGreenWaypoint = false;
        }
    }

    CellPos portalExit = level_->PortalExit(minion.cell);
    bool isPortalEntrance = level_->IsInside(portalExit.x, portalExit.y);

    if (isPortalEntrance && minion.stationTarget < 0) {
        Vector2 padCenter = {minion.cell.x + 0.5f, minion.cell.y + 0.5f};
        float moveX = padCenter.x - minion.position.x;
        float moveY = padCenter.y - minion.position.y;
        float distance = sqrt(moveX * moveX + moveY * moveY);
        float moveDistance = 2.6f * deltaTime;

        if (distance > moveDistance) {
            minion.position.x += moveX / distance * moveDistance;
            minion.position.y += moveY / distance * moveDistance;
            return;
        }

        if (minion.hasGreenWaypoint && minion.greenWaypoint == minion.cell) {
            minion.visitedGreenBeacons.push_back(minion.greenWaypoint);
            minion.hasGreenWaypoint = false;
        }

        minion.cell = portalExit;
        minion.position = {portalExit.x + 0.5f, portalExit.y + 0.5f};
        return;
    }

    CellPos target = level_->Start();

    if (minion.stationTarget >= 0) {
        const vector<CellPos>& stations = level_->CargoStations();
        CellPos station = stations[minion.stationTarget];
        target = station;

        if (minion.cell == station && IsAtCellCenter(minion.position, station)) {
            minion.stationsVisited.push_back(minion.stationTarget);
            minion.stationTarget = -1;
            minion.loadsCarried++;
        }
    }

    // After all stations, head back to the start
    if (minion.stationTarget < 0 && VisitedEveryStation(minion)) {
        target = level_->Start();
    } else if (minion.stationTarget < 0) {
        // Green beacons guide the next part of the trip
        if (!minion.hasGreenWaypoint) {
            int greenIndex = FindNextGreenWaypoint(minion.cell, minion.visitedGreenBeacons);

            if (greenIndex >= 0) {
                minion.greenWaypoint = beacons_[greenIndex].cell;
                minion.hasGreenWaypoint = true;
                target = minion.greenWaypoint;
            } else {
                target = level_->Start();
            }
        } else {
            target = minion.greenWaypoint;
        }
    }

    // Bring the cargo home when the minion reaches the start
    if (minion.stationTarget < 0 && !minion.hasGreenWaypoint &&
        target == level_->Start() && minion.cell == level_->Start() &&
        IsAtCellCenter(minion.position, level_->Start())) {
        if (minion.loadsCarried > 0) {
            stats_.loadsReturned += minion.loadsCarried;
        }
        
        if (VisitedEveryStation(minion)) {
            spawnMinions_ = false;
        }
        stats_.returnedHome++;
        minion.loadsCarried = 0;
        minion.alive = false;
        return;
    }

    vector<CellPos> route = Pathfinder::FindPath(*level_, minion.cell, target, beacons_);
    if (route.size() < 2) {
        // Finish the last part of the step if the cell changed at its edge
        if (minion.cell == target && !IsAtCellCenter(minion.position, target)) {
            Vector2 center = {target.x + 0.5f, target.y + 0.5f};
            float moveX = center.x - minion.position.x;
            float moveY = center.y - minion.position.y;
            float distance = sqrt(moveX * moveX + moveY * moveY);
            float moveDistance = 2.6f * deltaTime;

            if (distance <= moveDistance) {
                minion.position = center;
            } else {
                minion.position.x += moveX / distance * moveDistance;
                minion.position.y += moveY / distance * moveDistance;
            }
        }
        return;
    }

    CellPos nextCell = route[1];

    float speed = 2.6f;
    if (level_->At(nextCell.x, nextCell.y) == Tile::Water) {
        speed = 1.7f;
    }

    Vector2 targetPosition = {nextCell.x + 0.5f, nextCell.y + 0.5f};
    float moveX = targetPosition.x - minion.position.x;
    float moveY = targetPosition.y - minion.position.y;
    float distance = sqrt(moveX * moveX + moveY * moveY);
    float moveDistance = speed * deltaTime;

    if (distance <= moveDistance) {
        minion.position = targetPosition;
        minion.cell = nextCell;

        return;
    }

    minion.position.x += moveX / distance * moveDistance;
    minion.position.y += moveY / distance * moveDistance;
}

// ==================== SCORE ====================
// Finish the run and calculate the final score
void Game::FinishRun() {
    stats_.survivalRate = stats_.spawned > 0
        ? static_cast<float>(stats_.returnedHome) / stats_.spawned
        : 0.0f;

    // Use the run time, survival rate and beacons for the final score
    float time = max(0.001f, stats_.elapsedTime);
    float score = 1000.0f * stats_.loadsReturned / time;

    // More run beacon cost lowers the score without crushing it to zero
    float beaconMultiplier = BeaconCostScale / (BeaconCostScale + runBeaconCost_);
    score *= beaconMultiplier;

    if (stats_.survivalRate >= 0.75f) {
        score *= 1.75f;
    }

    stats_.score = static_cast<int>(score);
    won_ = true;
    running_ = false;
    SetNotice("Level complete · Press R to try again");
}

// ==================== GAME UPDATE ====================
// Move active minions and clean up the ones that are done
void Game::Update(float deltaTime) {
    if (!running_ || paused_ || !started_ || !level_) {
        return;
    }
    
    if (level_->HasBridgeEvent()) {
        float remaining = level_->BridgeCollapseRemaining();
        if (!bridgeWarningShown_ && remaining > 0.0f &&
            remaining <= level_->Definition().bridgeWarningSeconds) {
            SetNotice("Bridge collapsing soon · Find another route");
            bridgeWarningShown_ = true;
        }

        if (level_->Update(deltaTime)) {
            SetNotice("Bridge collapsed · Routes have changed");
        }
    }

    if (spawnMinions_) {
        UpdateSpawner(deltaTime);
    }

    notice_.timeLeft = max(0.0f, notice_.timeLeft - deltaTime);
    stats_.elapsedTime += deltaTime;

    for (Minion& minion : minions_) {
        if (minion.alive) {
            UpdateMinion(minion, deltaTime);
        }
    }
    for (int i = static_cast<int>(minions_.size()) - 1; i >= 0; i--) {
        if (!minions_[i].alive) {
            minions_.erase(minions_.begin() + i);
        }
    }
    
    if (minions_.empty() && !spawnMinions_) {
        FinishRun();
    }
    
}

// ==================== BEACON PLACEMENT ====================
// Check the map before adding a beacon
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

    beacons_.push_back(MakeBeacon(selectedBeaconType_, cell));
    if (started_) {
        // Run placements add cost and removing them will not refund it
        runBeaconCost_ += BeaconCost(selectedBeaconType_);
    }

    if (selectedBeaconType_ == GreenBeaconType) {
        for (Minion& minion : minions_) {
            minion.hasGreenWaypoint = false;
            minion.visitedGreenBeacons.clear();
        }
    }
}

// ==================== BEACON REMOVAL ====================
// Green beacons get a fresh route when one is removed
void Game::RemoveBeacon(CellPos cell) {
    for (int i = static_cast<int>(beacons_.size()) - 1; i >= 0; i--) {
        if (beacons_[i].cell == cell) {
            int removedType = beacons_[i].type;
            beacons_.erase(beacons_.begin() + i);
            SetNotice("Beacon removed");

            if (removedType == GreenBeaconType) {
                for (Minion& minion : minions_) {
                    minion.hasGreenWaypoint = false;
                    minion.visitedGreenBeacons.clear();
                }
            }
            return;
        }
    }
}

// ==================== PANEL INPUT ====================
// Handle clicks on the buttons beside the map
void Game::HandlePanelClick(Vector2 mouse) {
    if (IsInsideRectangle(mouse, ControlButtonX, StartButtonY, ControlButtonWidth, ControlButtonHeight)) {
        StartRun();
    } else if (IsInsideRectangle(mouse, ControlButtonX, GiveUpButtonY, ControlButtonWidth, ControlButtonHeight)) {
        GiveUp();
    }
}

// ==================== PLAYER INPUT ====================
// Read keys and mouse clicks for this frame
void Game::HandleInput() {
    if (IsKeyPressed(KEY_PAGE_UP)) {
        ChangeLevel(-1);
        return;
    }

    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        ChangeLevel(1);
        return;
    }

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

    if (IsKeyPressed(KEY_ONE)) selectedBeaconType_ = GreenBeaconType;
    if (IsKeyPressed(KEY_TWO)) selectedBeaconType_ = BlueBeaconType;
    if (IsKeyPressed(KEY_THREE)) selectedBeaconType_ = RedBeaconType;

    if (IsKeyPressed(KEY_ENTER)) {
        StartRun();
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
