#pragma once

#include "raylib.h"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace tri {
// ==================== GAME SETTINGS ====================
// Grid size, beacon IDs and screen layout are shared across the game
constexpr int GridWidth = 24;
constexpr int GridHeight = 16;
constexpr int CellSize = 40;
constexpr int BoardLeft = 32;
constexpr int BoardTop = 88;
constexpr int SidePanelWidth = 280;
constexpr int WindowWidth = GridWidth * CellSize + BoardLeft * 2 + SidePanelWidth;
constexpr int FooterHeight = 76;
constexpr int WindowHeight = GridHeight * CellSize + BoardTop + FooterHeight;
constexpr int CargoAreaRadius = 1;
constexpr int BeaconAreaRadius = 2;
constexpr int BaseScore = 10000;
constexpr int GreenBeaconType = 0;
constexpr int BlueBeaconType = 1;
constexpr int RedBeaconType = 2;
constexpr int ControlPanelX = BoardLeft + GridWidth * CellSize + 18;
constexpr int ControlPanelWidth = SidePanelWidth - 24;
constexpr int ControlButtonX = ControlPanelX + 10;
constexpr int ControlButtonWidth = ControlPanelWidth - 20;
constexpr int StartButtonY = BoardTop + 490;
constexpr int GiveUpButtonY = BoardTop + 535;
constexpr int ControlButtonHeight = 34;

// These helpers keep the game view centered and clicks lined up
inline float GameViewScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / WindowWidth;
    float heightScale = static_cast<float>(GetScreenHeight()) / WindowHeight;
    return widthScale < heightScale ? widthScale : heightScale;
}

inline Vector2 GameViewOffset() {
    float scale = GameViewScale();
    Vector2 offset;
    offset.x = (GetScreenWidth() - WindowWidth * scale) / 2.0f;
    offset.y = (GetScreenHeight() - WindowHeight * scale) / 2.0f;
    return offset;
}

inline Vector2 ScreenToGamePosition(Vector2 position) {
    float scale = GameViewScale();
    Vector2 offset = GameViewOffset();
    position.x = (position.x - offset.x) / scale;
    position.y = (position.y - offset.y) / scale;
    return position;
}

// ==================== SHARED GAME DATA ====================
// These small types hold the things the game passes around
// One grid spot with an x and y position
struct CellPos {
    int x = 0;
    int y = 0;

    bool operator==(const CellPos& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const CellPos& other) const {
        return !(*this == other);
    }
};

// One minion and the route details it keeps for itself
struct Minion {
    Vector2 position{};
    CellPos cell{};
    std::vector<CellPos> visitedGreenBeacons;
    std::vector<int> stationsVisited;
    int stationTarget = -1;
    int loadsCarried = 0;
    CellPos greenWaypoint{};
    bool hasGreenWaypoint = false;
    bool alive = true;
};

// One colored beacon placed by the player
struct Beacon {
    CellPos cell{};
    Color color{};
    int type = 0;
};

// Ground types that can show up on a map
enum class Tile { Floor, Wall, Water, CargoStation, Hazard };

struct GameStats {
    // The HUD uses these values to show run progress
    int spawned = 0;
    int returnedHome = 0;
    int lost = 0;
    int loadsReturned = 0;
    float elapsedTime = 0.0f;
    float scoreTime = 0.0f;
    float survivalRate = 0.0f;
    int score = 0;
};

struct Notice {
    // Short messages shown above the map
    std::string message;
    float timeLeft = 0.0f;
};

struct LevelDefinition {
    // One map and its beacon limits loaded from a Tiled file
    std::string name;
    std::array<std::array<Tile, GridWidth>, GridHeight> terrain{};
    CellPos start{};
    int greenBeaconLimit = 1;
    int blueBeaconLimit = 0;
    int redBeaconLimit = 0;
};

// Read all valid maps from the levels folder
std::vector<LevelDefinition> LoadLevelCatalog(const std::filesystem::path& directory);
}
