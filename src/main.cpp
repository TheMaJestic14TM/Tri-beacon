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

    // Load the menu and gameplay music once
    InitAudioDevice();
    Music menuMusic = LoadMusicStream("assets/audio/menu.mp3");
    Music gameplayMusic = LoadMusicStream("assets/audio/gameplay.mp3");
    menuMusic.looping = true;
    gameplayMusic.looping = true;
    SetMusicVolume(menuMusic, 1.0f / 6.0f);
    SetMusicVolume(gameplayMusic, 1.0f / 8.0f);

    tri::Game game;
    bool menuMusicActive = game.IsInMainMenu();

    if (IsMusicValid(menuMusic)) {
        PlayMusicStream(menuMusic);
    }

    // Read input, update the game and draw the screen each frame
    while (!WindowShouldClose()) {
        game.HandleInput();
        game.Update(min(GetFrameTime(), 0.05f));

        // Switch tracks when the player moves between the menu and a level
        bool inMainMenu = game.IsInMainMenu();
        if (inMainMenu != menuMusicActive) {
            if (IsMusicValid(menuMusic)) StopMusicStream(menuMusic);
            if (IsMusicValid(gameplayMusic)) StopMusicStream(gameplayMusic);

            if (inMainMenu && IsMusicValid(menuMusic)) {
                PlayMusicStream(menuMusic);
            } else if (!inMainMenu && IsMusicValid(gameplayMusic)) {
                PlayMusicStream(gameplayMusic);
            }

            menuMusicActive = inMainMenu;
        }

        // Music streams need an update every frame to keep playing
        if (inMainMenu && IsMusicValid(menuMusic)) {
            UpdateMusicStream(menuMusic);
        } else if (!inMainMenu && IsMusicValid(gameplayMusic)) {
            UpdateMusicStream(gameplayMusic);
        }

        tri::DrawGame(game);
    }

    // Free the audio streams before closing the audio device
    if (IsMusicValid(menuMusic)) UnloadMusicStream(menuMusic);
    if (IsMusicValid(gameplayMusic)) UnloadMusicStream(gameplayMusic);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
