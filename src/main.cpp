/** @file main.cpp
 * @brief Starts raylib and runs the main game loop
 */
#include "Game.h"
#include "Renderer.h"
#include "raylib.h"
#include <algorithm>

using namespace std;

// ==================== GAME LOOP ====================
int main() {
    // Set up the window and keep each frame moving smoothly
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(min(tri::WindowWidth, 1100), min(tri::WindowHeight, 680), "TRI BEACON");
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    int windowWidth = min(tri::WindowWidth, monitorWidth - 40);
    int windowHeight = min(tri::WindowHeight, monitorHeight - 80);
    SetWindowSize(windowWidth, windowHeight);
    SetWindowPosition((monitorWidth - windowWidth) / 2, (monitorHeight - windowHeight) / 2);
    SetTargetFPS(60);
    tri::Game game;

    // Read input, update the game and draw the screen each frame
    while (!WindowShouldClose()) {
        game.HandleInput();
        game.Update(min(GetFrameTime(), 0.05f));
        tri::DrawGame(game);
    }
    CloseWindow();
    return 0;
}
