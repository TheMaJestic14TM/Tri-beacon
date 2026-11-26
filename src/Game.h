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
    int RunBeaconCost() const { return runBeaconCost_; }
    int StationCount() const;
    bool CanStart() const;
    bool IsPaused() const { return paused_; }
    bool HasStarted() const { return started_; }
    bool IsRunning() const { return running_; }
    bool HasWon() const { return won_; }
    bool IsInScoringPhase() const { return scoringPhase_; }
    float ScoringPhaseTimeLeft() const { return scoringPhaseTimeLeft_; }
    bool IsInMainMenu() const { return menuOpen_; }
    int SelectedLevelIndex() const { return currentLevelIndex_; }
    int LevelCount() const { return static_cast<int>(levelCatalog_.size()); }
    const std::string& SelectedLevelName() const {
        static const std::string noLevel = "No levels found";
        return levelCatalog_.empty() ? noLevel : levelCatalog_[currentLevelIndex_].name;
    }
    int BestScore() const;

private:
    // Small helpers for the current run
    void SetNotice(const std::string& message);
    void StartSelectedLevel();
    void ReturnToMenu();
    void ChangeMenuLevel(int direction);
    void LoadBestScores();
    void SaveBestScores() const;
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
    std::vector<LevelDefinition> levelCatalog_;
    int currentLevelIndex_ = 0;
    std::vector<int> bestScores_;
    std::unique_ptr<Level> level_;
    std::vector<Minion> minions_;
    std::vector<Beacon> beacons_;
    GameStats stats_{};
    Notice notice_{};
    int selectedBeaconType_ = GreenBeaconType;
    int runBeaconCost_ = 0;
    bool running_ = true;
    bool menuOpen_ = true;
    bool started_ = false;
    bool spawnMinions_ = false;
    bool scoringPhase_ = false;
    bool paused_ = false;
    bool won_ = false;
    float spawnTimer_ = 0.0f;
    float scoringPhaseTimeLeft_ = 0.0f;
    int scoringPhaseSpawned_ = 0;
    int scoringPhaseReturned_ = 0;
    bool bridgeWarningShown_ = false;
};
}
