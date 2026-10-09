#include "Game.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>

#ifndef TRI_BEACON_LEVEL_DIR
#define TRI_BEACON_LEVEL_DIR "levels"
#endif

namespace tri {
using namespace std;

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
    stats_ = {};
    spawnTimer_ = 0.0f;
    notice_ = {};
    selectedBeacon_ = 1;
    started_ = false;
    paused_ = false;
    running_ = true;
    won_ = false;

    // Green stays put while the player sets up blue and red
    beacons_[0] = {level_->Goal(), {74, 218, 143, 255}, "GOAL", true};
    beacons_[1] = {{-1, -1}, {75, 173, 255, 255}, "BLUE", false};
    beacons_[2] = {{-1, -1}, {255, 100, 103, 255}, "RED", false};
}

// Pop a quick message above the board
void Game::SetNotice(const string& message) {
    notice_.message = message;
    notice_.timeLeft = 3.0f;
}

// Put the next minion right on the start tile
void Game::SpawnPawn() {
    CellPos start = level_->Start();
    Vector2 position = {start.x + 0.5f, start.y + 0.5f};

    pawns_.push_back({position, start});
    stats_.deployed++;
}

// Pick a crystal for a minion that is still empty handed
CellPos Game::ChooseCargoTarget(const Pawn& pawn) const {
    if (pawn.carryingCargo) {
        return level_->Start();
    }

    vector<CellPos> cargo = level_->AvailableCrystals();
    if (cargo.empty()) {
        return level_->Goal();
    }

    CellPos bestTarget = cargo[0];
    float bestCost = 1000000.0f;

    // Go for close cargo first and lean toward the green goal after that
    for (CellPos crystal : cargo) {
        vector<CellPos> route = Pathfinder::FindPath(
            *level_, pawn.cell, crystal, beacons_[1], beacons_[2]);

        if (route.empty()) {
            continue;
        }

        float cost = static_cast<float>(route.size());
        cost += level_->Distance(crystal, level_->Goal()) * 0.2f;

        if (cost < bestCost) {
            bestCost = cost;
            bestTarget = crystal;
        }
    }

    return bestTarget;
}

// Move one minion along and see what it runs into
void Game::UpdatePawn(Pawn& pawn, float deltaTime) {
    pawn.cell = {static_cast<int>(pawn.position.x), static_cast<int>(pawn.position.y)};

    // Step onto a hazard and this minion is out
    if (level_->At(pawn.cell.x, pawn.cell.y) == Tile::Hazard) {
        pawn.alive = false;
        stats_.lost++;
        return;
    }

    // Grab cargo and count the trip home when it makes it back
    if (level_->CollectCrystal(pawn.cell)) {
        pawn.carryingCargo = true;
        stats_.cargoCollected++;
    }

    if (pawn.carryingCargo && pawn.cell == level_->Start()) {
        pawn.alive = false;
        stats_.rescued++;
        return;
    }

    CellPos target = ChooseCargoTarget(pawn);
    vector<CellPos> route = Pathfinder::FindPath(
        *level_, pawn.cell, target, beacons_[1], beacons_[2]);

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

// Keep the level moving once the player hits Enter
void Game::Update(float deltaTime) {
    if (!running_ || paused_ || !started_ || !level_) {
        return;
    }

    const LevelDefinition& rules = level_->Definition();
    stats_.elapsedSeconds += deltaTime;
    spawnTimer_ += deltaTime;
    notice_.timeLeft = max(0.0f, notice_.timeLeft - deltaTime);

    // Send out minions at the pace this map asks for
    if (spawnTimer_ >= rules.pawnSpawnSeconds && stats_.deployed < rules.pawnLimit) {
        spawnTimer_ = 0.0f;
        SpawnPawn();
    }

    for (Pawn& pawn : pawns_) {
        if (pawn.alive) {
            UpdatePawn(pawn, deltaTime);
        }
    }

    // Call it a win once enough cargo and minions make it home
    if (stats_.cargoCollected >= rules.cargoGoal && stats_.rescued >= rules.rescueGoal) {
        won_ = true;
        running_ = false;
        SetNotice("Lesson complete · Press R to play again");
        return;
    }

    int livingPawns = 0;
    for (const Pawn& pawn : pawns_) {
        if (pawn.alive) {
            livingPawns++;
        }
    }

    // No minions left and no more spawns means the run is over
    if (livingPawns == 0 && stats_.deployed >= rules.pawnLimit) {
        running_ = false;
        SetNotice("No pawns left · Press R to try again");
    }
}

// Check the keys and clicks the player just made
void Game::HandleInput() {
    if (IsKeyPressed(KEY_R)) {
        Reset();
        return;
    }

    if (!running_) {
        return;
    }

    // Once the run is going, the player can pause or restart
    if (started_) {
        if (IsKeyPressed(KEY_SPACE)) {
            paused_ = !paused_;
        }
        return;
    }

    if (IsKeyPressed(KEY_ONE)) selectedBeacon_ = 0;
    if (IsKeyPressed(KEY_TWO)) selectedBeacon_ = 1;
    if (IsKeyPressed(KEY_THREE)) selectedBeacon_ = 2;

    // Drop the selected blue or red beacon onto the map
    if (selectedBeacon_ != 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        int boardRight = BoardLeft + GridWidth * CellSize;
        int boardBottom = BoardTop + GridHeight * CellSize;

        bool insideBoard = mouse.x >= BoardLeft && mouse.x < boardRight;
        insideBoard = insideBoard && mouse.y >= BoardTop && mouse.y < boardBottom;

        if (insideBoard) {
            CellPos cell;
            cell.x = static_cast<int>((mouse.x - BoardLeft) / CellSize);
            cell.y = static_cast<int>((mouse.y - BoardTop) / CellSize);

            if (level_->IsPathable(cell.x, cell.y)) {
                beacons_[selectedBeacon_].cell = cell;
                beacons_[selectedBeacon_].active = true;
            }
        }
    }

    // Enter locks in the plan and sends the minions out
    if (IsKeyPressed(KEY_ENTER)) {
        started_ = true;
        SetNotice("Minions are on the move");
    }
}
}
