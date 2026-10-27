#include "Renderer.h"
#include <string>

namespace tri {
using namespace std;

namespace {
// A few colors we use all over the board and UI
const Color Background = {15, 21, 32, 255};
const Color Panel = {23, 31, 45, 255};
const Color Text = {226, 235, 245, 255};
const Color Muted = {137, 153, 174, 255};
const Color Green = {105, 225, 162, 255};
const Color Blue = {75, 173, 255, 255};
const Color Red = {255, 100, 103, 255};

// Find the middle of a cell so everything lines up nicely
Vector2 CellCenter(CellPos cell) {
    Vector2 center;
    center.x = BoardLeft + cell.x * CellSize + CellSize / 2.0f;
    center.y = BoardTop + cell.y * CellSize + CellSize / 2.0f;
    return center;
}

// Draw a beacon with a letter so we can tell them apart
void DrawBeacon(const Beacon& beacon, int selectedType) {
    Vector2 center = CellCenter(beacon.cell);
    float outlineSize = 16.0f;

    if (selectedType == beacon.type) {
        outlineSize = 21.0f;
    }

    DrawCircleV(center, 19, ColorAlpha(beacon.color, 0.18f));
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), outlineSize, beacon.color);
    DrawCircleV(center, 10, beacon.color);

    const char* letter = "G";
    if (beacon.type == 1) letter = "B";
    if (beacon.type == 2) letter = "R";
    DrawText(letter, static_cast<int>(center.x) - 4, static_cast<int>(center.y) - 7, 14, Background);
}

// Draw one tile and add a little mark for special ground
void DrawTile(Tile tile, int gridX, int gridY) {
    int screenX = BoardLeft + gridX * CellSize;
    int screenY = BoardTop + gridY * CellSize;
    Color fill = {27, 37, 51, 255};

    if (tile == Tile::Wall) fill = {54, 65, 79, 255};
    if (tile == Tile::Water) fill = {28, 81, 111, 255};
    if (tile == Tile::Hazard) fill = {193, 76, 47, 255};
    if (tile == Tile::Crystal) fill = {113, 86, 52, 255};

    DrawRectangle(screenX + 1, screenY + 1, CellSize - 2, CellSize - 2, fill);
    DrawRectangleLines(screenX, screenY, CellSize, CellSize, Color{36, 48, 64, 255});

    if (tile == Tile::Water) {
        DrawCircle(screenX + 20, screenY + 20, 5, Color{83, 177, 213, 255});
        DrawCircle(screenX + 29, screenY + 12, 2, Color{83, 177, 213, 255});
    }
    if (tile == Tile::Hazard) {
        DrawCircle(screenX + 20, screenY + 20, 8, Color{255, 176, 79, 255});
        DrawCircle(screenX + 15, screenY + 15, 3, Color{255, 226, 130, 255});
    }
    if (tile == Tile::Crystal) {
        DrawPoly({screenX + 20.0f, screenY + 20.0f}, 4, 8, 45, Color{255, 214, 106, 255});
    }
}

// Put the level objective and current numbers across the top
void DrawHeader(const Game& game) {
    DrawRectangle(0, 0, WindowWidth, BoardTop, Panel);
    DrawText("TRI / BEACON", 32, 19, 25, Text);
    DrawText("Visit all stations, bring cargo back", 34, 51, 14, Muted);

    const Level* level = game.CurrentLevel();
    int cargoGoal = CargoGoal;
    if (level) {
        cargoGoal = level->Definition().cargoGoal;
    }

    const GameStats& stats = game.Stats();
    DrawText("CARGO", 370, 17, 11, Muted);
    DrawText(TextFormat("%d / %d", stats.cargoDelivered, cargoGoal), 370, 36, 19, Green);

    DrawText("HOME", 500, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.returned), 500, 36, 19, Green);

    DrawText("LOST", 610, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.lost), 610, 36, 19, Red);

    int minutes = static_cast<int>(stats.elapsedSeconds) / 60;
    int seconds = static_cast<int>(stats.elapsedSeconds) % 60;
    DrawText("TIME", 705, 17, 11, Muted);
    DrawText(TextFormat("%02d:%02d", minutes, seconds), 705, 36, 19, Text);

    if (game.HasWon()) {
        DrawText("SCORE", 810, 17, 11, Muted);
        DrawText(TextFormat("%d", stats.score), 810, 36, 19, Color{255, 214, 106, 255});
    }
}

// Draw the map first, then stack beacons and minions on top
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
    for (int y = 0; y < GridHeight; y++) {
        for (int x = 0; x < GridWidth; x++) {
            DrawTile(tiles[y][x], x, y);
        }
    }

    // Make the spawn spot easy to find
    CellPos start = level->Start();
    DrawCircleV(CellCenter(start), 12, Color{208, 221, 235, 255});
    DrawText("START", BoardLeft + start.x * CellSize + 3, BoardTop + start.y * CellSize + 29, 9, Background);

    // Beacons show up once the player drops them on the map
    for (const Beacon& beacon : game.Beacons()) {
        DrawBeacon(beacon, game.SelectedBeaconType());
    }

    // Put living minions over the tiles so they stay visible
    for (const Pawn& pawn : game.Pawns()) {
        if (!pawn.alive) {
            continue;
        }

        Vector2 position;
        position.x = BoardLeft + pawn.position.x * CellSize;
        position.y = BoardTop + pawn.position.y * CellSize;

        Color pawnColor = {230, 240, 250, 255};
        if (pawn.cargoCount > 0) {
            pawnColor = {255, 214, 106, 255};
        }

        DrawCircleV(position, 7, pawnColor);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 8, Color{22, 30, 42, 255});
    }
}

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

// Keep the beacon guide and controls right beside the map
void DrawControlPanel(const Game& game) {
    int x = ControlPanelX;
    int y = BoardTop;

    DrawRectangle(x, y, ControlPanelWidth, GridHeight * CellSize, Panel);
    DrawRectangleLines(x, y, ControlPanelWidth, GridHeight * CellSize, Color{52, 68, 88, 255});

    int line = y + 17;
    DrawText("THE JOB", x + 14, line, 17, Text);
    line += 29;
    DrawText(TextFormat("Deliver %d cargo", game.CurrentLevel() ? game.CurrentLevel()->Definition().cargoGoal : CargoGoal), x + 14, line, 13, Muted);
    line += 21;
    DrawText(TextFormat("Visit all %d stations", game.StationCount()), x + 14, line, 13, Muted);
    line += 21;
    DrawText("Then get back to START", x + 14, line, 13, Muted);
    line += 27;

    DrawText("BEACONS", x + 14, line, 17, Text);
    line += 29;
    DrawText(TextFormat("GREEN  %d / %d  Global waypoint", game.BeaconCount(0), game.BeaconLimit(0)), x + 14, line, 11, Green);
    line += 21;
    DrawText(TextFormat("BLUE  %d / %d  Lowers cost in 5x5", game.BeaconCount(1), game.BeaconLimit(1)), x + 14, line, 11, Blue);
    line += 21;
    DrawText(TextFormat("RED  %d / %d  Raises cost in 5x5", game.BeaconCount(2), game.BeaconLimit(2)), x + 14, line, 11, Red);
    line += 27;

    DrawText("HOW TO PLAY", x + 14, line, 17, Text);
    line += 29;
    DrawText("1 green  2 blue  3 red", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Left click adds a beacon", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Right click removes one", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Each minion visits all stations", x + 14, line, 12, Muted);
    line += 19;
    DrawText("then brings cargo home", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Green guides each trip", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Blue and red nudge the route", x + 14, line, 12, Muted);
    line += 19;
    DrawText("You can edit while they move", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Click again to send another", x + 14, line, 12, Muted);
    line += 19;
    DrawText("Cargo stations never run out", x + 14, line, 12, Muted);

    DrawButton(StartButtonY, "START RUN  (ENTER)", game.CanStart(), Green);
    DrawButton(ReleaseButtonY, "RELEASE ONE  (N)", game.CanRelease(), Blue);
    DrawButton(GiveUpButtonY, "GIVE UP  (ESC)", game.HasStarted() && game.IsRunning(), Red);

    if (!game.HasStarted() && game.BeaconCount(0) == 0) {
        DrawText("Place a green beacon to start", x + 14, StartButtonY - 22, 11, Red);
    }
}

// Add the little legend and controls along the bottom
void DrawFooter() {
    int y = BoardTop + GridHeight * CellSize + 13;

    DrawCircle(BoardLeft + 6, y + 6, 5, Color{230, 240, 250, 255});
    DrawText("MINIONS", BoardLeft + 18, y, 11, Muted);

    DrawCircle(BoardLeft + 110, y + 6, 5, Color{255, 214, 106, 255});
    DrawText("CARGO", BoardLeft + 122, y, 11, Muted);

    DrawCircle(BoardLeft + 214, y + 6, 5, Color{255, 110, 77, 255});
    DrawText("HAZARD", BoardLeft + 226, y, 11, Muted);

    DrawText("ENTER START   N RELEASE   RIGHT CLICK REMOVE   SPACE PAUSE   F11 FULLSCREEN   R RESET", BoardLeft + 365, y, 11, Muted);
}

// Leave a message up while it matters or after the run ends
void DrawNotice(const Game& game) {
    const Notice& notice = game.CurrentNotice();
    if (notice.timeLeft <= 0.0f && game.IsRunning()) {
        return;
    }

    if (notice.message.empty()) {
        return;
    }

    DrawRectangle(BoardLeft, BoardTop - 34, 570, 25, Color{25, 37, 52, 245});
    DrawText(notice.message.c_str(), BoardLeft + 10, BoardTop - 30, 14, Text);
}

// Darken the board when the game is paused or finished
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

    const char* hint = "Press R to try again";
    int hintX = BoardLeft + GridWidth * CellSize / 2 - MeasureText(hint, 16) / 2;
    DrawText(hint, hintX, titleY + 44, 16, Muted);
}
}

// Pull the whole screen together for this frame
void DrawGame(const Game& game) {
    BeginDrawing();
    ClearBackground(Background);

    DrawHeader(game);
    DrawBoard(game);
    DrawFooter();
    DrawNotice(game);
    DrawEndOverlay(game);
    DrawControlPanel(game);

    EndDrawing();
}
}
