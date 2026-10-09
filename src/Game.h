#pragma once

#include "GameTypes.h"
#include "Level.h"
#include <memory>

namespace tri {
// Keeps track of the level, minions, beacons and player input
class Game {
public:
    Game();

    // Give the current map a clean restart
    void Reset();

    // Keep the world moving and listen for player input
    void Update(float deltaTime);
    void HandleInput();

    // The renderer reads these to show what's going on
    const Level* CurrentLevel() const { return level_.get(); }
    const std::vector<Pawn>& Pawns() const { return pawns_; }
    const std::array<Beacon, 3>& Beacons() const { return beacons_; }
    const GameStats& Stats() const { return stats_; }
    const Notice& CurrentNotice() const { return notice_; }
    int SelectedBeacon() const { return selectedBeacon_; }
    bool IsPaused() const { return paused_; }
    bool HasStarted() const { return started_; }
    bool IsRunning() const { return running_; }
    bool HasWon() const { return won_; }

private:
    // These helpers keep the bigger bits from getting messy
    void SpawnPawn();
    void SetNotice(const std::string& message);
    CellPos ChooseCargoTarget(const Pawn& pawn) const;
    void UpdatePawn(Pawn& pawn, float deltaTime);

    // The current map and everything that can change during a run
    std::unique_ptr<Level> level_;
    std::vector<Pawn> pawns_;
    std::array<Beacon, 3> beacons_{};
    GameStats stats_{};
    Notice notice_{};
    float spawnTimer_ = 0.0f;
    int selectedBeacon_ = 1;
    bool running_ = true;
    bool started_ = false;
    bool paused_ = false;
    bool won_ = false;
};
}
