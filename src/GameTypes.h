#pragma once

#include "raylib.h"
#include <array>
#include <string>
#include <vector>
#include <filesystem>

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
constexpr int CargoGoal = 6;
constexpr int RescueGoal = 5;
constexpr int PawnLimit = 32;

struct CellPos {
    // One cell means one square on the map
    int x = 0;
    int y = 0;
    bool operator==(const CellPos& other) const { return x == other.x && y == other.y; }
    bool operator!=(const CellPos& other) const { return !(*this == other); }
};

struct Pawn {
    // This can slide between cells instead of jumping tile by tile
    Vector2 position{};
    CellPos cell{};
    bool carryingCargo = false;
    bool alive = true;
};

struct Beacon {
    // The player drops blue and red before sending minions out
    CellPos cell{};
    Color color{};
    const char* label = "";
    bool active = false;
};

// The kind of ground sitting in a map cell
enum class Tile { Floor, Wall, Water, Crystal, Hazard };

struct GameStats {
    // These counts show progress and tell us when the level is done
    int deployed = 0;
    int rescued = 0;
    int lost = 0;
    int cargoCollected = 0;
    float elapsedSeconds = 0.0f;
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
    CellPos goal{};
    float pawnSpawnSeconds = 1.15f;
    int pawnLimit = PawnLimit;
    int cargoGoal = CargoGoal;
    int rescueGoal = RescueGoal;
};

std::vector<LevelDefinition> LoadLevelCatalog(const std::filesystem::path& directory);
}
