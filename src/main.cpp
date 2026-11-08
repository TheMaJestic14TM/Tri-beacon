#include "Game.h"
#include "Renderer.h"
#include "raylib.h"
#include <algorithm>

using namespace std;

// ==================== GAME LOOP ====================
int main() {
    // Set up the window and keep each frame moving smoothly
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(tri::WindowWidth, tri::WindowHeight, "TRI / BEACON");
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
