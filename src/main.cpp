#include "raylib.h"
#include <cmath>
#include <iostream>



int main()
{
	// Initialize the window
	InitWindow(300, 300, "Tri-Beacon");
	
	// Windowsize, we should play around with this
	SetWindowSize(512, 512);
	//ToggleFullscreen();
	SetTargetFPS(60);

	while (!WindowShouldClose()) // Can be closed whith Esc
	{
		//Deltatime
		float deltaTime = GetFrameTime();
		
		BeginDrawing();

		ClearBackground(Color(WHITE));
		DrawCircle(256,256,32,Color(BLACK));

		DrawFPS(10, 10);//show FPS in the top left corner
		
		EndDrawing();
	}

	CloseWindow();

	return 0;
}