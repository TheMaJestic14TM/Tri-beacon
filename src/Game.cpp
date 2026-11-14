#include "Game.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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

// Used for clicks on the side panel buttons
bool IsInsideRectangle(Vector2 point, int x, int y, int width, int height) {
    return point.x >= x && point.x < x + width &&
        point.y >= y && point.y < y + height;
}
}

// ==================== GAME SETUP ====================
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
    beaconsUsedForScore_ = 0;
    spawnMinions_ = false;
    started_ = false;
    paused_ = false;
    running_ = true;
    won_ = false;
}

// Show a quick note when the player needs one
void Game::SetNotice(const string& message) {
    notice_.message = message;
    notice_.timeLeft = 3.0f;
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

// Read this map's limit for the selected beacon color
int Game::BeaconLimit(int type) const {
    if (!level_) {
        return 0;
    }

    const LevelDefinition& rules = level_->Definition();
    if (type == GreenBeaconType) return rules.greenBeaconLimit;
    if (type == BlueBeaconType) return rules.blueBeaconLimit;
    return rules.redBeaconLimit;
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
    beaconsUsedForScore_ = static_cast<int>(beacons_.size());
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
// Minions only notice a station when they enter its 3x3 area
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

    // Cargo only becomes a target when a minion gets close
    if (minion.stationTarget < 0) {
        int stationIndex = FindStationInRange(minion);
        if (stationIndex >= 0) {
            minion.stationTarget = stationIndex;
            minion.hasGreenWaypoint = false;
        }
    }

    CellPos target = level_->Start();

    if (minion.stationTarget >= 0) {
        const vector<CellPos>& stations = level_->CargoStations();
        CellPos station = stations[minion.stationTarget];
        target = station;

        if (minion.cell == station) {
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
        if (minion.hasGreenWaypoint && minion.cell == minion.greenWaypoint) {
            minion.visitedGreenBeacons.push_back(minion.greenWaypoint);
            minion.hasGreenWaypoint = false;
        }

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
        target == level_->Start() && minion.cell == level_->Start()) {
        if (minion.loadsCarried > 0) {
            stats_.loadsReturned += minion.loadsCarried;
        }
        
        spawnMinions_ = false;
        stats_.returnedHome++;
        minion.loadsCarried = 0;
        minion.alive = false;
        return;
    }

    vector<CellPos> route = Pathfinder::FindPath(*level_, minion.cell, target, beacons_);
    if (route.size() < 2) {
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
    float beaconCount = static_cast<float>(max(1, beaconsUsedForScore_));
    // Original score was: BaseScore * stats_.survivalRate / (stats_.scoreTime * beaconCount);
    std::cout <<  stats_.loadsReturned << " " << stats_.elapsedTime << " " << beaconCount << std::endl;
    float score = 1000 * stats_.loadsReturned / (stats_.elapsedTime * beaconCount);
    std::cout << score << std::endl;
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
    
    stats_.scoreTime += deltaTime;

    if (spawnMinions_)
    {
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
    
    if (minions_.empty() and !spawnMinions_)
    {
        FinishRun();
    }
    
}

// ==================== BEACON PLACEMENT ====================
// Check the map and level limits before adding a beacon
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
        beaconsUsedForScore_ = max(beaconsUsedForScore_, static_cast<int>(beacons_.size()));
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
