#include <stdio.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <functional>
#include "Globals.h"
#include "Complex.h"
#include "textOverlay.h"
#include "Fractal.h"
#include "ThreadPool.h"

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2_ttf.lib")

#undef main

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize SDL_ttf
				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

void close()
{
	//Destroy global things	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	TTF_CloseFont(gFont);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	SDL_Quit();
}

//for faster coloring, put hues into a table
void initColorTable(Uint32 *table)
{
	int i = 0;
	int c;

	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF0000 + 0x100 * c; //red-yellow
	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF0000 - 0x10000 * c + 0xFF00; //yellow-green
	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF00 + c; //green-cyan
	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF00 - 0x100 * c + 0xFF; //cyan-blue
	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF + 0x10000 * c; //blue-magenta
	for (c = 0; c < 256; c++, i++)
		table[i] = 0xFF - c + 0xFF0000; //magenta-red
}

//divide the window into rendering tiles
void initTiles()
{
	for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE)
	{
		for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE)
		{
			SDL_Rect rect;
			rect.x = x;
			rect.y = y;
			rect.w = ((SCREEN_WIDTH - x) < TILE_SIZE ? SCREEN_WIDTH - x : TILE_SIZE);
			rect.h = ((SCREEN_HEIGHT - y) < TILE_SIZE ? SCREEN_HEIGHT - y : TILE_SIZE);
			tiles.push_back(rect);

			SDL_Surface *surf = SDL_CreateRGBSurface(0, rect.w, rect.h, 32,
				0,0,0,0);
			SDL_SetSurfaceRLE(surf, true); //optimize surfaces
			surfaces.push_back(surf);
		}
	}
}

void handleEvents(complex *offset, double *zoom, unsigned *MaxIterations, complex *K, bool *update, bool *quit)
{
	//Event handler
	SDL_Event e;

	//Handle events on queue
	while (SDL_PollEvent(&e) != 0)
	{
		//User requests quit
		if (e.type == SDL_QUIT)
		{
			*quit = true;
		}
	}

	*update = false;
	if (state[SDL_SCANCODE_KP_MINUS]) { if (*MaxIterations > 0) { (*MaxIterations) -= 10; *update = true; } }
	else if (state[SDL_SCANCODE_KP_PLUS]) { (*MaxIterations) += 10; *update = true; }

	if (state[SDL_SCANCODE_DOWN]) { *zoom *= 1.2; *update = true; }
	else if (state[SDL_SCANCODE_UP]) { *zoom *= 0.9; *update = true; }

	if (state[SDL_SCANCODE_KP_8]) { offset->im -= 0.1*(*zoom); *update = true; }
	else if (state[SDL_SCANCODE_KP_2]) { offset->im += 0.1*(*zoom); *update = true; }

	if (state[SDL_SCANCODE_KP_4]) { offset->re -= 0.1*(*zoom); *update = true; }
	else if (state[SDL_SCANCODE_KP_6]) { offset->re += 0.1*(*zoom); *update = true; }

	if (state[SDL_SCANCODE_KP_7]) { K->re += 0.01; *update = true; }
	else if (state[SDL_SCANCODE_KP_1]) { K->re -= 0.01; *update = true; }

	if (state[SDL_SCANCODE_KP_9]) { K->im += 0.01;  *update = true; }
	else if (state[SDL_SCANCODE_KP_3]) { K->im -= 0.01;  *update = true; }
}

int main()
{
	//Start up SDL and create window
	printf("Initializing SDL... ");
	if (init()) printf("success\n");

	initColorTable(color_table);
	initTiles();

	complex K;
	K.re = -0.7;
	K.im = 0.27015;
	complex offset;
	offset.re = 0;
	offset.im = 0;

	unsigned interval = 100;
	unsigned StartIterations = 100;
	//unsigned MinIterations = 0;
	unsigned MaxIterations = StartIterations;
	double zoom = 1;

	MandelRenderer Mandel;
	Mandel.update(zoom ,offset);
	JuliaRenderer Julia;
	Julia.update(zoom, offset, K);

	textOverlay text;
	gFont = TTF_OpenFont("DigitalDream.ttf", 14);
	text.initText(gRenderer, gFont);

	//Main loop flags
	bool quit = false;
	bool update = true;

	while (!quit)
	{
		if (update)
		{
			//fill screen with black
			for (auto &t : surfaces)
			{
				SDL_FillRect(t, NULL, 0);
			}
			
			//start drawing from beginning
			//MinIterations = 0;
			MaxIterations = StartIterations;

			Mandel.update(zoom, offset);
			//Julia.update(zoom, offset, K);

			update = false;
		}

		handleEvents(&offset, &zoom, &StartIterations, &K, &update, &quit);


		//queue tasks
		ThreadPool pool(THREAD_COUNT);
		for (unsigned i = 0; i < tiles.size(); i++)
		{
			std::function<void()> f = std::bind(&FractalRenderer::render, &Mandel, i, MaxIterations);
			pool.Enqueue(f);
		}
		pool.ShutDown();

		//combine surfaces to one
		SDL_Surface *screen = SDL_GetWindowSurface(gWindow);
	
		for (unsigned i = 0; i < tiles.size(); i++)
		{
			SDL_BlitSurface(surfaces[i], NULL, screen, &tiles[i]);
		}
		SDL_BlitSurface(text.printText(offset, zoom, MaxIterations), NULL, screen, NULL);

		SDL_UpdateWindowSurface(gWindow);
		SDL_FreeSurface(screen);

		MaxIterations += interval; //render more iterations next time
	}
	close();
}