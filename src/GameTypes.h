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
constexpr int FooterHeight = 76;
constexpr int WindowHeight = GridHeight * CellSize + BoardTop + FooterHeight;
constexpr int CargoAreaRadius = 1;
constexpr int BeaconAreaRadius = 2;
constexpr int BaseScore = 10000;
constexpr int ControlPanelX = BoardLeft + GridWidth * CellSize + 18;
constexpr int ControlPanelWidth = SidePanelWidth - 24;
constexpr int ControlButtonX = ControlPanelX + 10;
constexpr int ControlButtonWidth = ControlPanelWidth - 20;
constexpr int StartButtonY = BoardTop + 490;
constexpr int ReleaseButtonY = BoardTop + 535;
constexpr int GiveUpButtonY = BoardTop + 580;
constexpr int ControlButtonHeight = 34;

// Scale the fixed game layout to fit the current window
inline float GameViewScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / WindowWidth;
    float heightScale = static_cast<float>(GetScreenHeight()) / WindowHeight;
    return widthScale < heightScale ? widthScale : heightScale;
}

// Keep the game centered when the window has a different shape
inline Vector2 GameViewOffset() {
    float scale = GameViewScale();
    Vector2 offset;
    offset.x = (GetScreenWidth() - WindowWidth * scale) / 2.0f;
    offset.y = (GetScreenHeight() - WindowHeight * scale) / 2.0f;
    return offset;
}

// Convert a screen click back to the fixed game layout
inline Vector2 ScreenToGamePosition(Vector2 position) {
    float scale = GameViewScale();
    Vector2 offset = GameViewOffset();
    position.x = (position.x - offset.x) / scale;
    position.y = (position.y - offset.y) / scale;
    return position;
}

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
    std::vector<CellPos> visitedGreenBeacons;
    std::vector<int> cargoStationIndexes;
    int targetStationIndex = -1;
    int cargoCount = 0;
    CellPos greenWaypoint{};
    bool hasGreenWaypoint = false;
    bool alive = true;
};

struct Beacon {
    // The player can add and remove these while the level is running
    CellPos cell{};
    Color color{};
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
};

std::vector<LevelDefinition> LoadLevelCatalog(const std::filesystem::path& directory);
}
