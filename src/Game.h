#pragma once

#include "GameTypes.h"
#include "Level.h"
#include <memory>

namespace tri {
// Owns the current run and connects the map, minions and input
class Game {
public:
    Game();

    // Main controls called by the game loop
    void Reset();
    void Update(float deltaTime);
    void HandleInput();
    void StartRun();
    void GiveUp();

    // Read-only data used by the renderer
    const Level* CurrentLevel() const { return level_.get(); }
    const std::vector<Minion>& Minions() const { return minions_; }
    const std::vector<Beacon>& Beacons() const { return beacons_; }
    const GameStats& Stats() const { return stats_; }
    const Notice& CurrentNotice() const { return notice_; }
    int SelectedBeaconType() const { return selectedBeaconType_; }
    int BeaconCount(int type) const;
    int BeaconLimit(int type) const;
    int StationCount() const;
    bool CanStart() const;
    bool IsPaused() const { return paused_; }
    bool HasStarted() const { return started_; }
    bool IsRunning() const { return running_; }
    bool HasWon() const { return won_; }

private:
    // Small helpers for the current run
    void SetNotice(const std::string& message);
    int FindStationInRange(const Minion& minion) const;
    bool HasVisitedStation(const Minion& minion, int stationIndex) const;
    bool VisitedEveryStation(const Minion& minion) const;
    int FindNextGreenWaypoint(CellPos from, const std::vector<CellPos>& visited) const;
    void UpdateMinion(Minion& minion, float deltaTime);
    void FinishRun();
    void PlaceBeacon(CellPos cell);
    void RemoveBeacon(CellPos cell);
    void HandlePanelClick(Vector2 mouse);
    void SpawnMinion();
    void UpdateSpawner(float deltaTime);

    // Current map and things placed or moving on it
    std::unique_ptr<Level> level_;
    std::vector<Minion> minions_;
    std::vector<Beacon> beacons_;
    GameStats stats_{};
    Notice notice_{};
    int selectedBeaconType_ = GreenBeaconType;
    int beaconsUsedForScore_ = 0;
    bool running_ = true;
    bool started_ = false;
    bool spawnMinions_ = false;
    bool paused_ = false;
    bool won_ = false;
    float spawnTimer_ = 0.0f;
};
}
