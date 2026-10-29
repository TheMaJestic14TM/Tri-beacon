#pragma once

#include "GameTypes.h"
#include "Level.h"
#include <memory>

namespace tri {
// Keeps track of the level, minions, beacons and player input
class Game {
public:
    Game();

    // Reset the level and clear the current run
    void Reset();
    void Update(float deltaTime);
    void HandleInput();
    void StartRun();
    void ReleasePawn();
    void GiveUp();

    // The renderer reads these to show what's going on
    const Level* CurrentLevel() const { return level_.get(); }
    const std::vector<Pawn>& Pawns() const { return pawns_; }
    const std::vector<Beacon>& Beacons() const { return beacons_; }
    const GameStats& Stats() const { return stats_; }
    const Notice& CurrentNotice() const { return notice_; }
    int SelectedBeaconType() const { return selectedBeaconType_; }
    int BeaconCount(int type) const;
    int BeaconLimit(int type) const;
    int StationCount() const;
    bool CanStart() const;
    bool CanRelease() const;
    bool IsPaused() const { return paused_; }
    bool HasStarted() const { return started_; }
    bool IsRunning() const { return running_; }
    bool HasWon() const { return won_; }

private:
    // Small helpers used by the game
    void SpawnPawn();
    void SetNotice(const std::string& message);
    int FindNearbyCargoStation(CellPos from) const;
    int FindNextGreenBeacon(CellPos from, const std::vector<CellPos>& visited) const;
    void UpdatePawn(Pawn& pawn, float deltaTime);
    void PlaceBeacon(CellPos cell);
    void RemoveBeacon(CellPos cell);
    void HandlePanelClick(Vector2 mouse);

    // The current map and everything that can change during a run
    std::unique_ptr<Level> level_;
    std::vector<Pawn> pawns_;
    // This list holds every beacon the player has placed
    std::vector<Beacon> beacons_;
    GameStats stats_{};
    Notice notice_{};
    int selectedBeaconType_ = 0;
    int maxBeaconsUsed_ = 0;
    bool running_ = true;
    bool started_ = false;
    bool paused_ = false;
    bool won_ = false;
};
}
