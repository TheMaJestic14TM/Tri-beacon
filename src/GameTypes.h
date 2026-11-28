#pragma once

#include "raylib.h"
#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace tri {
/** @brief Shared settings and data types used by the game */
// ==================== GAME SETTINGS ====================
// Grid size, beacon IDs and screen layout are shared across the game
/** @brief Number of map cells across */
constexpr int GridWidth = 32;
/** @brief Number of map cells down */
constexpr int GridHeight = 22;
/** @brief Pixel size of one map cell in the game view */
constexpr int CellSize = 32;
/** @brief Left edge of the game board */
constexpr int BoardLeft = 32;
/** @brief Top edge of the game board */
constexpr int BoardTop = 72;
/** @brief Width of the side panel */
constexpr int SidePanelWidth = 280;
/** @brief Width of the virtual game view */
constexpr int WindowWidth = GridWidth * CellSize + BoardLeft * 2 + SidePanelWidth;
/** @brief Height reserved for the keyboard hints */
constexpr int FooterHeight = 52;
/** @brief Height of the virtual game view */
constexpr int WindowHeight = GridHeight * CellSize + BoardTop + FooterHeight;
/** @brief Radius of the 5 by 5 cargo pickup area */
constexpr int CargoAreaRadius = 2;
/** @brief Radius of a beacon's effect area */
constexpr int BeaconAreaRadius = 2;
// At this cost, the beacon part of the score is cut in half
/** @brief Beacon cost value used in the final score */
constexpr float BeaconCostScale = 25.0f;
/** @brief Wait time after the first full delivery before ending the run */
constexpr float FinalScoringSeconds = 5.0f;
/** @brief Beacon type used for green route waypoints */
constexpr int GreenBeaconType = 0;
/** @brief Beacon type that lowers nearby movement cost */
constexpr int BlueBeaconType = 1;
/** @brief Beacon type that raises nearby movement cost */
constexpr int RedBeaconType = 2;
constexpr int ControlPanelX = BoardLeft + GridWidth * CellSize + 18;
constexpr int ControlPanelWidth = SidePanelWidth - 24;
constexpr int ControlButtonX = ControlPanelX + 10;
constexpr int ControlButtonWidth = ControlPanelWidth - 20;
constexpr int StartButtonY = BoardTop + 490;
constexpr int GiveUpButtonY = BoardTop + 535;
constexpr int MainMenuButtonY = BoardTop + 580;
constexpr int ControlButtonHeight = 34;
constexpr int MenuCardWidth = 780;
constexpr int MenuCardHeight = 560;
constexpr int MenuCardX = (WindowWidth - MenuCardWidth) / 2;
constexpr int MenuCardY = (WindowHeight - MenuCardHeight) / 2;
constexpr int MenuPlayWidth = 260;
constexpr int MenuPlayHeight = 54;
constexpr int MenuPlayX = (WindowWidth - MenuPlayWidth) / 2;
constexpr int MenuPlayY = MenuCardY + 140;
constexpr int MenuArrowY = MenuCardY + 280;
constexpr int MenuLeftArrowX = MenuCardX + 90;
constexpr int MenuRightArrowX = MenuCardX + MenuCardWidth - 130;
constexpr int MenuBestScoreY = MenuCardY + 365;

/** @brief Get the scale that fits the game view inside the current window */
inline float GameViewScale() {
    float widthScale = static_cast<float>(GetScreenWidth()) / WindowWidth;
    float heightScale = static_cast<float>(GetScreenHeight()) / WindowHeight;
    return widthScale < heightScale ? widthScale : heightScale;
}

/** @brief Get the offset used to center the game view */
inline Vector2 GameViewOffset() {
    float scale = GameViewScale();
    Vector2 offset;
    offset.x = (GetScreenWidth() - WindowWidth * scale) / 2.0f;
    offset.y = (GetScreenHeight() - WindowHeight * scale) / 2.0f;
    return offset;
}

/**
 * @brief Convert a mouse position into game view coordinates
 * @param position Position in window coordinates
 * @return Position in game view coordinates
 */
inline Vector2 ScreenToGamePosition(Vector2 position) {
    float scale = GameViewScale();
    Vector2 offset = GameViewOffset();
    position.x = (position.x - offset.x) / scale;
    position.y = (position.y - offset.y) / scale;
    return position;
}

// ==================== SHARED GAME DATA ====================
// These small types hold the things the game passes around
/** @brief A cell position in the map grid */
struct CellPos {
    /** @brief Horizontal map cell */
    int x = 0;
    /** @brief Vertical map cell */
    int y = 0;

    /** @brief Check whether two map cells match */
    bool operator==(const CellPos& other) const {
        return x == other.x && y == other.y;
    }

    /** @brief Check whether two map cells differ */
    bool operator!=(const CellPos& other) const {
        return !(*this == other);
    }
};

/** @brief A minion and the route state it keeps during a run */
struct Minion {
    /** @brief Current position in map cell units */
    Vector2 position{};
    /** @brief Current grid cell */
    CellPos cell{};
    /** @brief Green beacons visited during this trip */
    std::vector<CellPos> visitedGreenBeacons;
    /** @brief Cargo stations visited during this trip */
    std::vector<int> stationsVisited;
    /** @brief Current cargo target, or -1 when there is none */
    int stationTarget = -1;
    /** @brief Number of cargo loads carried */
    int loadsCarried = 0;
    /** @brief Green beacon currently being followed */
    CellPos greenWaypoint{};
    /** @brief True while a green beacon is selected */
    bool hasGreenWaypoint = false;
    /** @brief False after this minion is lost */
    bool alive = true;
};

/** @brief A colored beacon placed on the map */
struct Beacon {
    /** @brief Grid cell occupied by this beacon */
    CellPos cell{};
    /** @brief Color used to draw the beacon */
    Color color{};
    /** @brief Green, blue, or red beacon type */
    int type = 0;
};

/** @brief Terrain types that can appear on a level */
enum class Tile {
    Floor,         /**< Normal walkable ground */
    Wall,          /**< Blocks minion movement */
    Water,         /**< Walkable ground that slows minions */
    CargoStation,  /**< Place where minions pick up cargo */
    Hazard,        /**< Dangerous ground that removes minions */
    Bridge,        /**< Bridge that can collapse during a run */
    PortalAEntry,  /**< One-way entrance to portal A */
    PortalAExit,   /**< Destination for portal A */
    PortalBEntry,  /**< One-way entrance to portal B */
    PortalBExit    /**< Destination for portal B */
};

/** @brief Numbers collected while a level is being played */
struct GameStats {
    // The HUD uses these values to show run progress
    /** @brief Minions created during the run */
    int spawned = 0;
    /** @brief Minions that returned to the start */
    int returnedHome = 0;
    /** @brief Minions lost during the run */
    int lost = 0;
    /** @brief Cargo loads delivered to the start */
    int loadsReturned = 0;
    /** @brief Time spent in the run, in seconds */
    float elapsedTime = 0.0f;
    /** @brief Share of minions that returned home */
    float survivalRate = 0.0f;
    /** @brief Final score for the level */
    int score = 0;
};

/** @brief A short message shown on the game screen */
struct Notice {
    // Short messages shown above the map
    /** @brief Message text */
    std::string message;
    /** @brief Time left before the message disappears */
    float timeLeft = 0.0f;
};

/** @brief Map data loaded from a Tiled JSON file */
struct LevelDefinition {
    // One map and its events loaded from a Tiled file
    /** @brief Name shown in the level menu */
    std::string name;
    /** @brief Fixed grid containing terrain for this map */
    std::array<std::array<Tile, GridWidth>, GridHeight> terrain{};
    /** @brief Minion spawn and return cell */
    CellPos start{};
    /** @brief Time before the bridge collapses, or zero when disabled */
    float bridgeCollapseSeconds = 0.0f;
    /** @brief Time before the bridge warning appears */
    float bridgeWarningSeconds = 5.0f;
};

/**
 * @brief Load all valid maps from a folder
 * @param directory Folder that contains Tiled map files
 * @return Valid level definitions sorted by file name
 */
std::vector<LevelDefinition> LoadLevelCatalog(const std::filesystem::path& directory);
}
