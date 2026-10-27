#pragma once

#include "raylib.h"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace tri {
// Shared sizes and the little bits of data the game passes around
constexpr int GridWidth = 24;
constexpr int GridHeight = 16;
constexpr int CellSize = 40;
constexpr int BoardLeft = 32;
constexpr int BoardTop = 88;
constexpr int SidePanelWidth = 280;
constexpr int WindowWidth = GridWidth * CellSize + BoardLeft * 2 + SidePanelWidth;
constexpr int WindowHeight = GridHeight * CellSize + BoardTop + 36;
constexpr int CargoGoal = 3;
constexpr int CargoAreaRadius = 1;
constexpr int BeaconAreaRadius = 2;
constexpr int BaseScore = 10000;
constexpr int ControlPanelX = BoardLeft + GridWidth * CellSize + 18;
constexpr int ControlPanelWidth = SidePanelWidth - 24;
constexpr int ControlButtonX = ControlPanelX + 10;
constexpr int ControlButtonWidth = ControlPanelWidth - 20;
constexpr int StartButtonY = BoardTop + 455;
constexpr int ReleaseButtonY = BoardTop + 500;
constexpr int GiveUpButtonY = BoardTop + 545;
constexpr int ControlButtonHeight = 36;

struct CellPos {
    // One cell means one square on the map
    int x = 0;
    int y = 0;

    bool operator==(const CellPos& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const CellPos& other) const {
        return !(*this == other);
    }
};

struct Pawn {
    // This can slide between cells instead of jumping tile by tile
    Vector2 position{};
    CellPos cell{};
    std::vector<bool> visitedStations;
    int cargoCount = 0;
    CellPos greenWaypoint{};
    bool hasGreenWaypoint = false;
    bool passedGreenWaypoint = false;
    bool alive = true;
};

struct Beacon {
    // The player can add and remove these while the level is running
    CellPos cell{};
    Color color{};
    const char* label = "";
    int type = 0;
};

enum class Tile { Floor, Wall, Water, Crystal, Hazard };

struct GameStats {
    // These counts show progress and tell us when the level is done
    int deployed = 0;
    int returned = 0;
    int lost = 0;
    int cargoDelivered = 0;
    float elapsedSeconds = 0.0f;
    int score = 0;
};

struct Notice {
    std::string message;
    float timeLeft = 0.0f;
};

struct LevelDefinition {
    // The map and its rules, loaded from a Tiled JSON file
    std::string name;
    std::array<std::array<Tile, GridWidth>, GridHeight> terrain{};
    CellPos start{};
    int greenBeaconLimit = 1;
    int blueBeaconLimit = 0;
    int redBeaconLimit = 0;
    int cargoGoal = CargoGoal;
};

std::vector<LevelDefinition> LoadLevelCatalog(const std::filesystem::path& directory);
}
