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

// Find the middle of a cell so everything lines up nicely
Vector2 CellCenter(CellPos cell) {
    Vector2 center;
    center.x = BoardLeft + cell.x * CellSize + CellSize / 2.0f;
    center.y = BoardTop + cell.y * CellSize + CellSize / 2.0f;
    return center;
}

// Draw a beacon with a letter so we can tell them apart
void DrawBeacon(const Beacon& beacon, int index, int selectedBeacon) {
    Vector2 center = CellCenter(beacon.cell);
    float outlineSize = 16.0f;

    if (selectedBeacon == index) {
        outlineSize = 21.0f;
    }

    DrawCircleV(center, 19, ColorAlpha(beacon.color, 0.18f));
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), outlineSize, beacon.color);
    DrawCircleV(center, 10, beacon.color);

    const char* letter = "G";
    if (index == 1) letter = "B";
    if (index == 2) letter = "R";

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

// Put the level name and score bits across the top
void DrawHeader(const Game& game) {
    DrawRectangle(0, 0, WindowWidth, BoardTop, Panel);
    DrawText("TRI / BEACON", 32, 19, 25, Text);

    const Level* level = game.CurrentLevel();
    DrawText("Collect cargo and bring minions back to START", 34, 51, 14, Muted);

    int rescueGoal = RescueGoal;
    int cargoGoal = CargoGoal;
    if (level) {
        rescueGoal = level->Definition().rescueGoal;
        cargoGoal = level->Definition().cargoGoal;
    }

    const GameStats& stats = game.Stats();
    DrawText("DELIVERED", 370, 17, 11, Muted);
    DrawText(TextFormat("%d / %d", stats.rescued, rescueGoal), 370, 36, 19, Color{105, 225, 162, 255});

    DrawText("CARGO", 490, 17, 11, Muted);
    DrawText(TextFormat("%d / %d", stats.cargoCollected, cargoGoal), 490, 36, 19, Color{255, 207, 112, 255});

    DrawText("LOST", 610, 17, 11, Muted);
    DrawText(TextFormat("%d", stats.lost), 610, 36, 19, Color{255, 125, 122, 255});

    int minutes = static_cast<int>(stats.elapsedSeconds) / 60;
    int seconds = static_cast<int>(stats.elapsedSeconds) % 60;
    DrawText("TIME", 705, 17, 11, Muted);
    DrawText(TextFormat("%02d:%02d", minutes, seconds), 705, 36, 19, Text);
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
    for (int i = 0; i < 3; i++) {
        const Beacon& beacon = game.Beacons()[i];
        if (beacon.active) {
            DrawBeacon(beacon, i, game.SelectedBeacon());
        }
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
        if (pawn.carryingCargo) {
            pawnColor = {255, 214, 106, 255};
        }

        DrawCircleV(position, 7, pawnColor);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 8, Color{22, 30, 42, 255});
    }
}

// Keep the beacon guide and controls right beside the map
void DrawControlPanel(const Game& game) {
    int x = BoardLeft + GridWidth * CellSize + 18;
    int y = BoardTop;
    int width = SidePanelWidth - 24;
    int height = GridHeight * CellSize;

    DrawRectangle(x, y, width, height, Panel);
    DrawRectangleLines(x, y, width, height, Color{52, 68, 88, 255});

    int line = y + 18;
    DrawText("BEACON BASICS", x + 14, line, 17, Text);
    line += 31;

    DrawText("Collect 3 cargo", x + 14, line, 13, Muted);
    line += 22;
    DrawText("Bring 3 minions back to START", x + 14, line, 13, Muted);
    line += 30;

    DrawText("BEACONS", x + 14, line, 17, Text);
    line += 31;
    DrawText("GREEN  Marks the goal", x + 14, line, 13, Color{105, 225, 162, 255});
    line += 22;
    DrawText("BLUE  Lowers nearby route cost", x + 14, line, 13, Color{75, 173, 255, 255});
    line += 22;
    DrawText("RED  Raises nearby route cost", x + 14, line, 13, Color{255, 100, 103, 255});
    line += 30;

    DrawText("PLAN BEFORE START", x + 14, line, 17, Text);
    line += 31;
    DrawText("1  Press 2, then click the", x + 14, line, 13, Muted);
    line += 22;
    DrawText("upper opening to place BLUE", x + 14, line, 13, Muted);
    line += 27;
    DrawText("2  Press 3, then click the", x + 14, line, 13, Muted);
    line += 22;
    DrawText("lower opening to place RED", x + 14, line, 13, Muted);
    line += 30;

    DrawText("The direct route crosses the", x + 14, line, 13, Text);
    line += 22;
    DrawText("orange hazard and kills minions", x + 14, line, 13, Text);
    line += 31;

    if (!game.HasStarted() && game.IsRunning()) {
        DrawRectangle(x + 10, line, width - 20, 48, Color{42, 54, 69, 255});
        DrawText("ENTER  Start the minions", x + 20, line + 9, 13, Color{255, 214, 106, 255});
        DrawText("Beacons lock once started", x + 20, line + 28, 11, Muted);
    } else {
        DrawText("SPACE  Pause or resume", x + 14, line, 13, Text);
        line += 22;
        DrawText("R  Restart the level", x + 14, line, 13, Text);
    }
}

// Add the little legend and controls along the bottom
void DrawFooter() {
    int y = BoardTop + GridHeight * CellSize + 13;

    DrawCircle(BoardLeft + 6, y + 6, 5, Color{230, 240, 250, 255});
    DrawText("PAWNS", BoardLeft + 18, y, 11, Muted);

    DrawCircle(BoardLeft + 110, y + 6, 5, Color{255, 214, 106, 255});
    DrawText("CARGO", BoardLeft + 122, y, 11, Muted);

    DrawCircle(BoardLeft + 214, y + 6, 5, Color{255, 110, 77, 255});
    DrawText("HAZARD", BoardLeft + 226, y, 11, Muted);

    DrawText("2 / 3 + CLICK PLACE   ENTER RUN   SPACE PAUSE   R RESET", BoardLeft + 365, y, 11, Muted);
}

// Leave a message up while it matters or after the run ends
void DrawNotice(const Game& game) {
    const Notice& notice = game.CurrentNotice();
    if (notice.timeLeft <= 0.0f && game.IsRunning()) {
        return;
    }

    DrawRectangle(BoardLeft, BoardTop - 34, 430, 25, Color{25, 37, 52, 245});
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

    const char* title = "MINIONS LOST";
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
