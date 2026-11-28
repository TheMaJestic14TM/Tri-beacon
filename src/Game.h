#pragma once

#include "GameTypes.h"
#include "Level.h"
#include <memory>

namespace tri {
/**
 * @brief Controls the menu, current level, minions, and run state
 * @details Minions follow unvisited green beacons and collect cargo stations
 * when they enter a station's 5 by 5 area. They return to the start after
 * visiting every station. Beacons affect the route cost used by A star.
 */
class Game {
public:
    /** @brief Load the level list and prepare the first level */
    Game();

    /** @brief Reset the current level and run data */
    void Reset();
    /**
     * @brief Update the level and active minions
     * @param deltaTime Time since the last update in seconds
     */
    void Update(float deltaTime);
    /** @brief Read keyboard and mouse input */
    void HandleInput();
    /** @brief Start the current level run */
    void StartRun();
    /** @brief End the current run without winning */
    void GiveUp();

    /** @brief Get the current level, or null when none is loaded */
    const Level* CurrentLevel() const { return level_.get(); }
    /** @brief Get all minions currently in the run */
    const std::vector<Minion>& Minions() const { return minions_; }
    /** @brief Get all beacons placed by the player */
    const std::vector<Beacon>& Beacons() const { return beacons_; }
    /** @brief Get the current run statistics */
    const GameStats& Stats() const { return stats_; }
    /** @brief Get the current short message */
    const Notice& CurrentNotice() const { return notice_; }
    /** @brief Get the type of beacon selected by the player */
    int SelectedBeaconType() const { return selectedBeaconType_; }
    /**
     * @brief Count placed beacons of one type
     * @param type Beacon type to count
     * @return Number of matching beacons
     */
    int BeaconCount(int type) const;
    /** @brief Get the beacon cost for this run */
    int RunBeaconCost() const { return runBeaconCost_; }
    /** @brief Get the number of cargo stations on the current level */
    int StationCount() const;
    /** @brief Check whether the player can start the run */
    bool CanStart() const;
    /** @brief Check whether the run is paused */
    bool IsPaused() const { return paused_; }
    /** @brief Check whether the run has started */
    bool HasStarted() const { return started_; }
    /** @brief Check whether the run is active */
    bool IsRunning() const { return running_; }
    /** @brief Check whether the player completed the level */
    bool HasWon() const { return won_; }
    /** @brief Check whether the final scoring period is active */
    bool IsInScoringPhase() const { return scoringPhase_; }
    /** @brief Get the time left in the final scoring period */
    float ScoringPhaseTimeLeft() const { return scoringPhaseTimeLeft_; }
    /** @brief Check whether the main menu is open */
    bool IsInMainMenu() const { return menuOpen_; }
    /** @brief Get the selected level's index */
    int SelectedLevelIndex() const { return currentLevelIndex_; }
    /** @brief Get the number of loaded levels */
    int LevelCount() const { return static_cast<int>(levelCatalog_.size()); }
    /** @brief Get the selected level name, or a message when no level is loaded */
    const std::string& SelectedLevelName() const {
        static const std::string noLevel = "No levels found";
        return levelCatalog_.empty() ? noLevel : levelCatalog_[currentLevelIndex_].name;
    }
    /** @brief Get the saved best score for the selected level */
    int BestScore() const;

private:
    /**
     * @brief Show a short message to the player
     * @param message Text to show
     */
    void SetNotice(const std::string& message);
    /** @brief Load the level selected in the menu */
    void StartSelectedLevel();
    /** @brief Return to the level selection menu */
    void ReturnToMenu();
    /**
     * @brief Change the selected menu level
     * @param direction -1 for the previous level or 1 for the next level
     */
    void ChangeMenuLevel(int direction);
    /** @brief Read saved scores from the local score file */
    void LoadBestScores();
    /** @brief Write best scores to the local score file */
    void SaveBestScores() const;
    /**
     * @brief Find an unvisited cargo station near a minion
     * @param minion Minion to check
     * @return Station index, or -1 when none is nearby
     */
    int FindStationInRange(const Minion& minion) const;
    /**
     * @brief Check whether a minion has visited one station
     * @param minion Minion to check
     * @param stationIndex Station index to find
     */
    bool HasVisitedStation(const Minion& minion, int stationIndex) const;
    /**
     * @brief Check whether a minion has visited every station
     * @param minion Minion to check
     */
    bool VisitedEveryStation(const Minion& minion) const;
    /**
     * @brief Find the next green beacon this minion has not visited
     * @param from Cell used to find the nearest waypoint
     * @param visited Green beacons already visited
     * @return Index of the next beacon, or -1 when none remains
     */
    int FindNextGreenWaypoint(CellPos from, const std::vector<CellPos>& visited) const;
    /**
     * @brief Move one minion and handle its cargo and route state
     * @param minion Minion to update
     * @param deltaTime Time since the last update in seconds
     */
    void UpdateMinion(Minion& minion, float deltaTime);
    /** @brief Calculate and save the score when the run ends */
    void FinishRun();
    /**
     * @brief Place the selected beacon on a map cell
     * @param cell Cell where the beacon should be placed
     */
    void PlaceBeacon(CellPos cell);
    /**
     * @brief Remove the beacon on a map cell
     * @param cell Cell to clear
     */
    void RemoveBeacon(CellPos cell);
    /**
     * @brief Handle clicks in the control panel
     * @param mouse Mouse position in game view coordinates
     */
    void HandlePanelClick(Vector2 mouse);
    /** @brief Create one minion at the level start */
    void SpawnMinion();
    /**
     * @brief Create minions while spawning is active
     * @param deltaTime Time since the last update in seconds
     */
    void UpdateSpawner(float deltaTime);

    /** @brief All level definitions shown in the menu */
    std::vector<LevelDefinition> levelCatalog_;
    /** @brief Index of the currently selected level */
    int currentLevelIndex_ = 0;
    /** @brief Best saved score for each level */
    std::vector<int> bestScores_;
    /** @brief Level currently loaded into the game */
    std::unique_ptr<Level> level_;
    /** @brief Minions currently in the run */
    std::vector<Minion> minions_;
    /** @brief Beacons placed on the current map */
    std::vector<Beacon> beacons_;
    /** @brief Statistics for the current run */
    GameStats stats_{};
    /** @brief Message currently shown on screen */
    Notice notice_{};
    /** @brief Type of beacon selected by the player */
    int selectedBeaconType_ = GreenBeaconType;
    /** @brief Beacon cost counted for the current run */
    int runBeaconCost_ = 0;
    /** @brief True while the run can receive updates */
    bool running_ = true;
    /** @brief True while the level menu is open */
    bool menuOpen_ = true;
    /** @brief True after the player starts the run */
    bool started_ = false;
    /** @brief True while minions are being created */
    bool spawnMinions_ = false;
    /** @brief True during the final scoring period */
    bool scoringPhase_ = false;
    /** @brief True while the game is paused */
    bool paused_ = false;
    /** @brief True after the level has been completed */
    bool won_ = false;
    /** @brief Time accumulated toward the next minion spawn */
    float spawnTimer_ = 0.0f;
    /** @brief Time left in the final scoring period */
    float scoringPhaseTimeLeft_ = 0.0f;
    /** @brief Minions spawned after the scoring period began */
    int scoringPhaseSpawned_ = 0;
    /** @brief Minions returned after the scoring period began */
    int scoringPhaseReturned_ = 0;
    /** @brief True after the bridge warning has been shown */
    bool bridgeWarningShown_ = false;
};
}
