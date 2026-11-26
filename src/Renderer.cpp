#include "Renderer.h"
#include <cmath>
#include <string>

namespace tri {
using namespace std;

namespace {
// ==================== COLORS ====================
// Shared colors keep the screen looking consistent
const Color Background = {15, 21, 32, 255};
const Color Panel = {23, 31, 45, 255};
const Color Text = {226, 235, 245, 255};
const Color Muted = {137, 153, 174, 255};
const Color Green = {105, 225, 162, 255};
const Color Blue = {75, 173, 255, 255};
const Color Red = {255, 100, 103, 255};

// ==================== DRAWING HELPERS ====================
// Find the center of a map cell in screen space
Vector2 CellCenter(CellPos cell) {
    Vector2 center;
    center.x = BoardLeft + cell.x * CellSize + CellSize / 2.0f;
    center.y = BoardTop + cell.y * CellSize + CellSize / 2.0f;
    return center;
}

// Draw one beacon and show its letter
void DrawBeacon(const Beacon& beacon, int selectedType) {
    Vector2 center = CellCenter(beacon.cell);
    float outlineSize = CellSize * 0.40f;

    if (selectedType == beacon.type) {
        outlineSize = CellSize * 0.49f;
    }

    DrawCircleV(center, CellSize * 0.43f, ColorAlpha(beacon.color, 0.18f));
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), outlineSize, beacon.color);
    DrawCircleV(center, CellSize * 0.25f, beacon.color);

    const char* letter = "G";
    if (beacon.type == BlueBeaconType) letter = "B";
    if (beacon.type == RedBeaconType) letter = "R";
    DrawText(letter, static_cast<int>(center.x) - 4, static_cast<int>(center.y) - 6, 12, Background);
}

// Draw the ground and its small marker
void DrawTile(Tile tile, int gridX, int gridY, double time) {
    int screenX = BoardLeft + gridX * CellSize;
    int screenY = BoardTop + gridY * CellSize;
    int centerX = screenX + CellSize / 2;
    int centerY = screenY + CellSize / 2;
    Color fill = {27, 37, 51, 255};

    if (tile == Tile::Wall) fill = {54, 65, 79, 255};
    if (tile == Tile::Water) fill = {28, 81, 111, 255};
    if (tile == Tile::Hazard) fill = {193, 76, 47, 255};
    if (tile == Tile::CargoStation) fill = {113, 86, 52, 255};
    if (tile == Tile::Bridge) fill = {32, 76, 105, 255};
    if (tile == Tile::PortalAEntry || tile == Tile::PortalAExit) fill = {25, 69, 91, 255};
    if (tile == Tile::PortalBEntry || tile == Tile::PortalBExit) fill = {59, 42, 99, 255};

    DrawRectangle(screenX + 1, screenY + 1, CellSize - 2, CellSize - 2, fill);
    DrawRectangleLines(screenX, screenY, CellSize, CellSize, Color{36, 48, 64, 255});

    if (tile == Tile::Water) {
        DrawLine(screenX + 6, centerY - 4, screenX + CellSize - 6, centerY - 4, Color{83, 177, 213, 255});
        DrawLine(screenX + 9, centerY + 4, screenX + CellSize - 9, centerY + 4, Color{83, 177, 213, 255});
    }
    if (tile == Tile::Hazard) {
        float flicker = static_cast<float>(sin(time * 7.0 + gridX * 0.8 + gridY)) * 2.0f;
        DrawCircle(centerX, centerY + 3, CellSize * 0.30f, Color{83, 34, 29, 255});
        DrawTriangle(
            {centerX - 10.0f, centerY + 8.0f},
            {centerX + flicker, centerY - 11.0f},
            {centerX + 10.0f, centerY + 8.0f},
            Color{255, 112, 42, 255});
        DrawTriangle(
            {centerX - 5.0f, centerY + 6.0f},
            {centerX + flicker * 0.5f, centerY - 5.0f},
            {centerX + 5.0f, centerY + 6.0f},
            Color{255, 218, 92, 255});
        DrawLine(centerX - 12, centerY + 11, centerX - 7, centerY + 8, Color{255, 91, 36, 255});
        DrawLine(centerX + 8, centerY + 10, centerX + 13, centerY + 7, Color{255, 91, 36, 255});
    }
    if (tile == Tile::CargoStation) {
        DrawPoly({static_cast<float>(centerX), static_cast<float>(centerY)}, 4, CellSize * 0.24f, 45, Color{255, 214, 106, 255});
    }
    if (tile == Tile::Bridge) {
        DrawRectangle(screenX + 4, screenY + 4, CellSize - 8, CellSize - 8, Color{112, 72, 45, 255});
        for (int plank = 0; plank < 3; plank++) {
            int plankY = screenY + 7 + plank * (CellSize - 14) / 3;
            DrawLine(screenX + 5, plankY, screenX + CellSize - 5, plankY, Color{204, 148, 91, 255});
        }
        DrawLine(screenX + 6, screenY + 5, screenX + 6, screenY + CellSize - 5, Color{230, 188, 117, 255});
        DrawLine(screenX + CellSize - 6, screenY + 5, screenX + CellSize - 6, screenY + CellSize - 5, Color{230, 188, 117, 255});
    }
    if (tile == Tile::PortalAEntry || tile == Tile::PortalAExit ||
        tile == Tile::PortalBEntry || tile == Tile::PortalBExit) {
        bool isPortalA = tile == Tile::PortalAEntry || tile == Tile::PortalAExit;
        bool isEntry = tile == Tile::PortalAEntry || tile == Tile::PortalBEntry;
        float pulse = isEntry ? static_cast<float>(sin(time * 4.0 + gridX + gridY)) * 2.0f : 0.0f;
        Color portalColor = isPortalA ? Color{94, 222, 255, 255} : Color{192, 118, 255, 255};
        DrawCircleLines(centerX, centerY, CellSize * 0.34f + pulse, portalColor);
        DrawCircleLines(centerX, centerY, CellSize * 0.20f, Color{223, 250, 255, 255});

        // Entrances have a hollow center and exits have a bright center
        if (isEntry) {
            DrawCircleLines(centerX, centerY, CellSize * 0.08f, portalColor);
        } else {
            DrawCircle(centerX, centerY, CellSize * 0.08f, portalColor);
        }

    }
}

// ==================== HEADER AND MAP ====================
// Show the current objective and run stats
void DrawHeader(const Game& game) {
    DrawRectangle(0, 0, WindowWidth, BoardTop, Panel);
    DrawText("TRI BEACON", 32, 25, 25, Text);

    const GameStats& stats = game.Stats();
    DrawText("LOADS HOME", 370, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.loadsReturned), 370, 36, 19, Green);

    DrawText("HOME", 500, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.returnedHome), 500, 36, 19, Green);

    DrawText("LOST", 610, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.lost), 610, 36, 19, Red);

    int minutes = static_cast<int>(stats.elapsedTime) / 60;
    int seconds = static_cast<int>(stats.elapsedTime) % 60;
    DrawText("TIME", 705, 17, 11, Muted);
    DrawText(TextFormat("%02d:%02d", minutes, seconds), 705, 36, 19, Text);

    if (game.IsInScoringPhase() && !game.HasWon()) {
        DrawText("FINAL", 810, 17, 11, Muted);
        DrawText(TextFormat("%d", static_cast<int>(ceil(game.ScoringPhaseTimeLeft()))),
            810, 36, 19, Color{255, 214, 106, 255});
    }

    if (game.HasWon()) {
        DrawText("RATE", 810, 17, 11, Muted);
        DrawText(TextFormat("%d%%", static_cast<int>(stats.survivalRate * 100.0f)), 810, 36, 19, Green);
        DrawText("SCORE", 900, 17, 11, Muted);
        DrawText(TextFormat("%d", stats.score), 900, 36, 19, Color{255, 214, 106, 255});
    }
}

// Draw the map first, then put the beacons and minions on top
void DrawBoard(const Game& game) {
    DrawRectangle(
        BoardLeft - 7,
        BoardTop - 7,
        GridWidth * CellSize + 14,
        GridHeight * CellSize + 14,
        Color{38, 49, 64, 255});

    const Level* level = game.CurrentLevel();
    if (!level) {
        return;
    }

    const Level::Grid& tiles = level->Tiles();
    double time = GetTime();
    for (int y = 0; y < GridHeight; y++) {
        for (int x = 0; x < GridWidth; x++) {
            DrawTile(tiles[y][x], x, y, time);

            // Flash the bridge while it is about to collapse
            bool bridgeWarning = level->HasBridgeEvent() && !level->BridgeCollapsed();
            bridgeWarning = bridgeWarning && level->BridgeCollapseRemaining() <= level->Definition().bridgeWarningSeconds;
            if (tiles[y][x] == Tile::Bridge && bridgeWarning) {
                float pulse = 0.4f + 0.6f * static_cast<float>(sin(time * 12.0));
                int tileX = BoardLeft + x * CellSize;
                int tileY = BoardTop + y * CellSize;
                DrawRectangleLines(tileX + 2, tileY + 2, CellSize - 4, CellSize - 4,
                    ColorAlpha(Color{255, 91, 65, 255}, pulse));
                DrawLine(tileX + 7, tileY + 7, tileX + CellSize - 8, tileY + CellSize - 8,
                    ColorAlpha(Color{255, 195, 102, 255}, pulse));
            }
        }
    }

    // A quick flash makes the bridge collapse easy to notice
    float collapseEffect = level->BridgeCollapseEffectTime();
    if (collapseEffect > 0.0f) {
        for (CellPos cell : level->BridgeCells()) {
            Vector2 center = CellCenter(cell);
            float radius = CellSize * (0.25f + (1.0f - collapseEffect) * 0.65f);
            Color flash = ColorAlpha(Color{255, 177, 72, 255}, collapseEffect);
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, flash);
            DrawLine(static_cast<int>(center.x - 9), static_cast<int>(center.y - 7),
                static_cast<int>(center.x + 8), static_cast<int>(center.y + 8), flash);
        }
    }

    CellPos start = level->Start();
    DrawCircleV(CellCenter(start), 12, Color{208, 221, 235, 255});

    for (const Beacon& beacon : game.Beacons()) {
        DrawBeacon(beacon, game.SelectedBeaconType());
    }

    for (const Minion& minion : game.Minions()) {
        if (!minion.alive) {
            continue;
        }

        Vector2 position;
        position.x = BoardLeft + minion.position.x * CellSize;
        position.y = BoardTop + minion.position.y * CellSize;

        Color minionColor = {230, 240, 250, 255};
        if (minion.loadsCarried > 0) {
            minionColor = {255, 214, 106, 255};
        }

        DrawCircleV(position, 7, minionColor);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 8, Color{22, 30, 42, 255});
    }
}

// ==================== CONTROLS ====================
// Draw a button and dim it when it cannot be used
void DrawButton(int y, const char* label, bool enabled, Color color) {
    Color fill = Color{40, 49, 63, 255};
    Color labelColor = Muted;

    if (enabled) {
        fill = Color{52, 69, 87, 255};
        labelColor = color;
    }

    DrawRectangle(ControlButtonX, y, ControlButtonWidth, ControlButtonHeight, fill);
    DrawRectangleLines(ControlButtonX, y, ControlButtonWidth, ControlButtonHeight, Color{77, 96, 116, 255});
    int textX = ControlButtonX + (ControlButtonWidth - MeasureText(label, 14)) / 2;
    DrawText(label, textX, y + 11, 14, labelColor);
}

// Draw one button on the main menu
void DrawMenuButton(int x, int y, int width, int height, const char* label, Color color) {
    DrawRectangle(x, y, width, height, Color{40, 55, 72, 255});
    DrawRectangleLines(x, y, width, height, color);
    int textX = x + (width - MeasureText(label, 18)) / 2;
    DrawText(label, textX, y + (height - 18) / 2, 18, color);
}

// Show the game title and let the player pick a level
void DrawMainMenu(const Game& game) {
    for (int x = 0; x < WindowWidth; x += CellSize) {
        DrawLine(x, 0, x, WindowHeight, Color{23, 31, 45, 100});
    }
    for (int y = 0; y < WindowHeight; y += CellSize) {
        DrawLine(0, y, WindowWidth, y, Color{23, 31, 45, 100});
    }

    DrawRectangle(MenuCardX, MenuCardY, MenuCardWidth, MenuCardHeight, Panel);
    DrawRectangleLines(MenuCardX, MenuCardY, MenuCardWidth, MenuCardHeight, Color{52, 68, 88, 255});

    const char* title = "TRI BEACON";
    DrawText(title, (WindowWidth - MeasureText(title, 38)) / 2, MenuCardY + 42, 38, Text);

    const char* subtitle = "PLAN THE ROUTE. BRING THE CARGO HOME.";
    DrawText(subtitle, (WindowWidth - MeasureText(subtitle, 15)) / 2, MenuCardY + 95, 15, Muted);

    DrawMenuButton(MenuPlayX, MenuPlayY, MenuPlayWidth, MenuPlayHeight, "PLAY", Green);

    DrawText("SELECT LEVEL", (WindowWidth - MeasureText("SELECT LEVEL", 14)) / 2,
        MenuCardY + 235, 14, Muted);

    DrawRectangle(MenuLeftArrowX, MenuArrowY, 48, 48, Color{40, 55, 72, 255});
    DrawRectangleLines(MenuLeftArrowX, MenuArrowY, 48, 48, Color{77, 96, 116, 255});
    DrawTriangle({static_cast<float>(MenuLeftArrowX + 30), static_cast<float>(MenuArrowY + 9)},
        {static_cast<float>(MenuLeftArrowX + 15), static_cast<float>(MenuArrowY + 24)},
        {static_cast<float>(MenuLeftArrowX + 30), static_cast<float>(MenuArrowY + 39)}, Green);

    DrawRectangle(MenuRightArrowX, MenuArrowY, 48, 48, Color{40, 55, 72, 255});
    DrawRectangleLines(MenuRightArrowX, MenuArrowY, 48, 48, Color{77, 96, 116, 255});
    DrawTriangle({static_cast<float>(MenuRightArrowX + 16), static_cast<float>(MenuArrowY + 39)},
        {static_cast<float>(MenuRightArrowX + 31), static_cast<float>(MenuArrowY + 24)},
        {static_cast<float>(MenuRightArrowX + 16), static_cast<float>(MenuArrowY + 9)}, Green);

    const char* levelName = game.SelectedLevelName().c_str();
    int nameSize = 25;
    DrawText(levelName, (WindowWidth - MeasureText(levelName, nameSize)) / 2,
        MenuArrowY + 2, nameSize, Text);

    const char* levelNumber = "NO LEVELS FOUND";
    if (game.LevelCount() > 0 && game.SelectedLevelIndex() == 0) {
        levelNumber = "TUTORIAL";
    } else if (game.LevelCount() > 0) {
        levelNumber = TextFormat("LEVEL %d OF %d", game.SelectedLevelIndex(), game.LevelCount() - 1);
    }
    DrawText(levelNumber, (WindowWidth - MeasureText(levelNumber, 12)) / 2,
        MenuArrowY + 34, 12, Blue);

    const char* bestScore = TextFormat("BEST SCORE  %d", game.BestScore());
    DrawText(bestScore, (WindowWidth - MeasureText(bestScore, 16)) / 2,
        MenuBestScoreY, 16, Color{255, 214, 106, 255});

    const char* controls = "LEFT / RIGHT TO SELECT     ENTER TO PLAY";
    DrawText(controls, (WindowWidth - MeasureText(controls, 12)) / 2,
        MenuCardY + MenuCardHeight - 61, 12, Muted);
    const char* fullscreen = "F11  FULLSCREEN";
    DrawText(fullscreen, (WindowWidth - MeasureText(fullscreen, 12)) / 2,
        MenuCardY + MenuCardHeight - 35, 12, Green);
}

// Show the objective, beacon guide and buttons
void DrawControlPanel(const Game& game) {
    int x = ControlPanelX;
    int y = BoardTop;

    DrawRectangle(x, y, ControlPanelWidth, GridHeight * CellSize, Panel);
    DrawRectangleLines(x, y, ControlPanelWidth, GridHeight * CellSize, Color{52, 68, 88, 255});

    int line = y + 17;
    DrawText("THE JOB", x + 14, line, 17, Text);
    line += 28;
    DrawText(TextFormat("Visit all %d stations", game.StationCount()), x + 14, line, 13, Muted);
    line += 20;
    DrawText("Then return to the start", x + 14, line, 13, Muted);
    line += 20;
    DrawText("Stations stay for every minion", x + 14, line, 13, Muted);
    line += 20;
    DrawText("Cargo range is 5x5", x + 14, line, 13, Muted);
    line += 24;

    DrawText("BEACONS", x + 14, line, 17, Text);
    line += 29;
    DrawText(TextFormat("GREEN     %d   COST 2", game.BeaconCount(GreenBeaconType)), x + 14, line, 12, Green);
    line += 17;
    DrawText("Guides the minions to a waypoint", x + 14, line, 11, Muted);
    line += 23;
    DrawText(TextFormat("BLUE       %d   COST 1", game.BeaconCount(BlueBeaconType)), x + 14, line, 12, Blue);
    line += 17;
    DrawText("Makes nearby paths cheaper", x + 14, line, 11, Muted);
    line += 23;
    DrawText(TextFormat("RED         %d   COST 1", game.BeaconCount(RedBeaconType)), x + 14, line, 12, Red);
    line += 17;
    DrawText("Makes nearby paths more costly", x + 14, line, 11, Muted);
    line += 24;

    DrawText("HOW TO PLAY", x + 14, line, 17, Text);
    line += 29;
    DrawText("1 green   2 blue   3 red", x + 14, line, 12, Muted);
    line += 20;
    DrawText("Left click places   Right click removes", x + 14, line, 11, Muted);
    line += 20;
    const Level* level = game.CurrentLevel();
    if (level && !level->PortalLinks().empty()) {
        DrawText("A IN sends to A OUT", x + 14, line, 11, Color{94, 222, 255, 255});
        line += 17;
        DrawText("B IN sends to B OUT", x + 14, line, 11, Color{192, 118, 255, 255});
        line += 20;
    }

    if (game.HasStarted()) {
        DrawText(TextFormat("RUN BEACON COST  %d", game.RunBeaconCost()), x + 14, line, 12, Color{255, 214, 106, 255});
        line += 20;
    } else {
        DrawText("Setup beacons are free", x + 14, line, 11, Muted);
        line += 20;
    }

    if (level && level->HasBridgeEvent() && level->BridgeCollapsed()) {
        DrawText("BRIDGE COLLAPSED", x + 14, line, 12, Red);
        line += 20;
    } else if (level && level->HasBridgeEvent() &&
        level->BridgeCollapseRemaining() <= level->Definition().bridgeWarningSeconds) {
        DrawText("BRIDGE IS UNSTABLE", x + 14, line, 12, Color{255, 153, 79, 255});
        line += 20;
    }

    DrawButton(StartButtonY, "START RUN  (ENTER)", game.CanStart(), Green);
    DrawButton(GiveUpButtonY, "GIVE UP", game.HasStarted() && game.IsRunning(), Red);
    DrawButton(MainMenuButtonY, "MAIN MENU  (M / ESC)", true, Blue);

    if (!game.HasStarted() && game.BeaconCount(GreenBeaconType) == 0) {
        DrawText("Place a green beacon to start", x + 14, StartButtonY - 22, 11, Red);
    }
}

// Keep the keyboard controls easy to scan
void DrawFooter() {
    int y = BoardTop + GridHeight * CellSize + 7;
    DrawRectangle(0, y - 4, WindowWidth, FooterHeight, Panel);

    const int groupX[4] = {82, 400, 718, 1036};
    const char* keys[4] = {"ENTER", "SPACE", "R", "F11"};
    const char* actions[4] = {"Run", "Pause", "Reset level", "Fullscreen"};

    for (int i = 0; i < 4; i++) {
        DrawRectangle(groupX[i], y + 9, 250, 36, Color{31, 42, 58, 255});
        DrawRectangleLines(groupX[i], y + 9, 250, 36, Color{52, 68, 88, 255});
        DrawText(keys[i], groupX[i] + 10, y + 20, 11, Green);
        DrawText(actions[i], groupX[i] + 84, y + 20, 11, Text);
    }
}

// Show short messages while they are still useful
void DrawNotice(const Game& game) {
    const Notice& notice = game.CurrentNotice();
    if (notice.timeLeft <= 0.0f && game.IsRunning()) {
        return;
    }

    if (notice.message.empty()) {
        return;
    }

    int noticeWidth = MeasureText(notice.message.c_str(), 12) + 20;
    int noticeY = BoardTop - 21;

    DrawRectangle(BoardLeft, noticeY, noticeWidth, 19, Color{25, 37, 52, 245});
    DrawText(notice.message.c_str(), BoardLeft + 10, noticeY + 3, 12, Text);
}

// Darken the map when the game is paused or over
void DrawEndOverlay(const Game& game) {
    if (!game.IsPaused() && game.IsRunning()) {
        return;
    }

    DrawRectangle(
        BoardLeft,
        BoardTop,
        GridWidth * CellSize,
        GridHeight * CellSize,
        Color{5, 9, 15, 105});

    const char* title = "GAVE UP";
    if (game.IsPaused()) title = "PAUSED";
    if (game.HasWon()) title = "LEVEL COMPLETE";

    int titleX = BoardLeft + GridWidth * CellSize / 2 - MeasureText(title, 32) / 2;
    int titleY = BoardTop + GridHeight * CellSize / 2 - 36;
    DrawText(title, titleX, titleY, 32, Text);
    
    const char* scoreText = TextFormat("SCORE: %d", game.Stats().score) ;
    int scoreX = BoardLeft + GridWidth * CellSize / 2 - MeasureText(scoreText, 16) / 2;
    int scoreY = titleY + 100;
    
    if (game.HasWon())
    {
        DrawText(scoreText, scoreX, scoreY, 16, Text);
    }

    const char* hint = "Press R to try again";
    int hintX = BoardLeft + GridWidth * CellSize / 2 - MeasureText(hint, 16) / 2;
    DrawText(hint, hintX, titleY + 44, 16, Muted);
}
}

// ==================== MAIN DRAW CALL ====================
// Draw one complete frame
void DrawGame(const Game& game) {
    BeginDrawing();
    ClearBackground(Background);

    Camera2D camera{};
    camera.offset = GameViewOffset();
    camera.zoom = GameViewScale();
    BeginMode2D(camera);

    if (game.IsInMainMenu()) {
        DrawMainMenu(game);
    } else {
        DrawHeader(game);
        DrawBoard(game);
        DrawFooter();
        DrawNotice(game);
        DrawEndOverlay(game);
        DrawControlPanel(game);
    }

    EndMode2D();
    EndDrawing();
}
}
