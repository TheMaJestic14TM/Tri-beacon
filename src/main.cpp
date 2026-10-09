#include "Game.h"
#include "Renderer.h"
#include "raylib.h"
#include <algorithm>

using namespace std;


int main() {
    // Every frame checks input, updates the world and draws it
    // Cap the frame time so minions do not jump after a pause
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(tri::WindowWidth, tri::WindowHeight, "TRI / BEACON - A living grid puzzle");
    SetTargetFPS(60);
    tri::Game game;
    while (!WindowShouldClose()) {
        game.HandleInput();
        game.Update(min(GetFrameTime(), 0.05f));
        tri::DrawGame(game);
    }
    CloseWindow();
    return 0;
}
