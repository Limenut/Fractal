#include <stdio.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2_ttf.lib")

#undef main

//global window and renderer
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font *gFont = NULL;

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 1000;

const unsigned THREAD_COUNT = 8;
const int TILE_SIZE = 512;

std::vector<SDL_Rect>tiles;
std::vector<std::thread>threads;

SDL_Color color_table[256 * 6];
const Uint8* state = SDL_GetKeyboardState(NULL);

int pixelBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];


class complex
{
public:
	double re;
	double im;
};

class coordinates
{
public:
	complex max;
	complex min;
	complex factor;
};

class textOverlay
{
public:
	textOverlay();
	SDL_Texture *tx[4];
	SDL_Rect rc[4];
	std::string str[4];
	SDL_Color color;
	TTF_Font *font;

	void initText();
	void printText(complex offset, double zoom, unsigned MaxIterations);
};

textOverlay::textOverlay() { initText(); }

void textOverlay::initText()
{
	gFont = TTF_OpenFont("DigitalDream.ttf", 14);

	for (int i = 0; i < 4; i++) { str[i] = " "; }
	color = { 0xFF, 0xFF, 0xFF };
	font = gFont;
	for (int i = 0; i < 4; i++)
	{
		rc[i].x = 0, rc[i].y = i * 20;
	}

	//varaa surface
	SDL_Surface *surf = NULL;

	for (int i = 0; i < 4; i++)
	{
		//laita teksti surffille ja talleta se tekstuuriin
		surf = TTF_RenderText_Solid(font, str[0].c_str(), color);
		tx[i] = SDL_CreateTextureFromSurface(gRenderer, surf);

		//surface pois
		SDL_FreeSurface(surf);

		//tekstuurin koko tallennetaan rektankkeliin
		SDL_QueryTexture(tx[i], NULL, NULL, &rc[i].w, &rc[i].h);
	}
}

void textOverlay::printText(complex offset, double zoom, unsigned MaxIterations)
{
	std::stringstream ss;
	ss.str("");
	ss << "x: " << offset.re;
	str[0] = ss.str();
	ss.str("");
	ss << "y: " << offset.im;
	str[1] = ss.str();
	ss.str("");
	ss << "zoom: " << zoom;
	str[2] = ss.str();
	ss.str("");
	ss << "max iterations: " << MaxIterations;
	str[3] = ss.str();

	SDL_Surface *surf = NULL;

	for (int i = 0; i < 4; i++)
	{
		surf = TTF_RenderText_Solid(font, str[i].c_str(), color);
		tx[i] = SDL_CreateTextureFromSurface(gRenderer, surf);

		SDL_FreeSurface(surf);

		SDL_QueryTexture(tx[i], NULL, NULL, &rc[i].w, &rc[i].h);

		SDL_RenderCopy(gRenderer, tx[i], NULL, &rc[i]);
	}
}

const complex MandelMin = {-2.0, -1.5};
const complex JuliaMin = { -1.5, -1.5 };
const complex PixelFactor = { 3.0 / (SCREEN_WIDTH - 1), 3.0 / (SCREEN_HEIGHT - 1) };
const double centerOFF = 0.5; //distance from origin to center of screen


void drawDotRain(unsigned x, unsigned y, unsigned i, unsigned i_max);
void drawDotCyclic(unsigned x, unsigned y, unsigned i);

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

void initColorTable()
{
	int i = 0;
	int c;

	for (c = 0; c < 256; c++, i++)
		color_table[i] = { 255, Uint8(c), 0 }; //red-yellow
	for (c = 0; c < 256; c++, i++)
		color_table[i] = { Uint8(255 - c), 255, 0 }; //yellow-green
	for (c = 0; c < 256; c++, i++)
		color_table[i] = { 0, 255, Uint8(c) }; //green-cyan
	for (c = 0; c < 256; c++, i++)
		color_table[i] = { 0, Uint8(255 - c), 255 }; //cyan-blue
	for (c = 0; c < 256; c++, i++)
		color_table[i] = { Uint8(c), 0, 255 }; //blue-magenta
	for (c = 0; c < 256; c++, i++)
		color_table[i] = { 255, 0, Uint8(255 - c) }; //magenta-red
}

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
		}
	}
}

void renderManPart(unsigned MinIterations, unsigned MaxIterations, double zoom, complex offset, SDL_Rect part)
{
	for (int y = part.y; y < part.y + part.h; ++y)
	{
		complex c;
		c.im = (MandelMin.im + y * PixelFactor.im) * zoom + offset.im;
		for (int x = part.x; x < part.x + part.w; ++x)
		{
			c.re = (MandelMin.re + centerOFF + x * PixelFactor.re) * zoom + offset.re - centerOFF;

			//cardioid-check
			double cardioid = (c.re - 0.25)*(c.re - 0.25) + c.im*c.im;
			if (cardioid*(cardioid + (c.re - 0.25)) < 0.25*c.im*c.im)
				continue;

			//first sphere-check
			if ((c.re + 1.0)*(c.re + 1.0) + c.im*c.im < (1.0 / 16.0))
				continue;

			complex Z;
			Z.re = c.re, Z.im = c.im;
			bool isInside = true;
			unsigned iterations;

			//escape time algorithm
			for (iterations = MinIterations; iterations < MaxIterations; ++iterations)
			{
				complex Z_2;
				Z_2.re = Z.re*Z.re, Z_2.im = Z.im*Z.im;

				if (Z_2.re + Z_2.im > 4)
				{
					isInside = false;
					break;
				}

				Z.im = 2 * Z.re*Z.im + c.im;
				Z.re = Z_2.re - Z_2.im + c.re;
			}

			if (!isInside) pixelBuffer[y][x] = iterations;
		}
	}
}

void renderJulPart(unsigned MinIterations, unsigned MaxIterations, double zoom, complex offset, complex K, SDL_Rect part)
{
	for (int y = part.y; y < part.y + part.h; ++y)
	{
		for (int x = part.x; x < part.x + part.w; ++x)
		{
			if (pixelBuffer[y][x] >= 0) continue;

			complex Z;
			Z.re = (JuliaMin.re + x * PixelFactor.re) * zoom + offset.re;
			Z.im = (JuliaMin.im + y * PixelFactor.im) * zoom + offset.im;
			bool isInside = true;
			unsigned iterations;

			for (iterations = MinIterations; iterations < MaxIterations; ++iterations)
			{
				complex Z_2;
				Z_2.re = Z.re*Z.re, Z_2.im = Z.im*Z.im;

				
				if (Z_2.re + Z_2.im > 4)
				{
					isInside = false;
					break;
				}
				Z.im = 2 * Z.re*Z.im + K.im;
				Z.re = Z_2.re - Z_2.im + K.re;
			}
			if (!isInside) pixelBuffer[y][x] = iterations;
		}
	}
}

void wholeRender()
{
	//Clear screen
	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(gRenderer);

	for (int y = 0; y < SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x < SCREEN_WIDTH; ++x)
		{
			if (pixelBuffer[y][x] >= 0)
			{
				drawDotCyclic(x, y, pixelBuffer[y][x]);
			}
		}
	}
}

void renderPart(SDL_Rect part)
{
	//SDL_Texture* texture;
	//SDL_SetRenderTarget(gRenderer, texture);
	for (int y = part.y; y < part.h; ++y)
	{
		for (int x = part.x; x < part.w; ++x)
		{
			if (pixelBuffer[y][x] >= 0)
			{
				int hue = (pixelBuffer[y][x] * 12) % (256 * 6);

				SDL_Color color = color_table[hue];

				SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, 0xFF);
				SDL_RenderDrawPoint(gRenderer, x, y);
			}
		}
	}
}

void drawDotRain(unsigned x, unsigned y, unsigned i, unsigned i_max)
{
	int hue = (int)(((double)i / (double)i_max) * 256.0 * 6.0 + 0.5);

	SDL_Color color = color_table[hue];

	SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, 0xFF);
	SDL_RenderDrawPoint(gRenderer, x, y);
}

void drawDotCyclic(unsigned x, unsigned y, unsigned i)
{
	int hue = (i * 12) % (256 * 6);

	SDL_Color color = color_table[hue];

	SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, 0xFF);
	SDL_RenderDrawPoint(gRenderer, x, y);
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
	else if (state[SDL_SCANCODE_KP_1]) {K->re -= 0.01; *update = true; }

	if (state[SDL_SCANCODE_KP_9]) {K->im += 0.01;  *update = true; }
	else if (state[SDL_SCANCODE_KP_3]) { K->im -= 0.01;  *update = true; }
}

int main()
{
	//Start up SDL and create window
	printf("Initializing SDL... ");
	if (init()) printf("success\n");

	initColorTable();
	initTiles();

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threads.push_back(std::thread());
	}

	complex K;
	K.re = -0.7;
	K.im = 0.27015;
	complex offset;
	offset.re = 0;
	offset.im = 0;

	unsigned interval = 50;
	unsigned MinIterations = 0;
	unsigned MaxIterations = MinIterations + interval;
	double zoom = 1;

	textOverlay text;

	//Main loop flag
	bool quit = false;
	bool update = true;

	memset(pixelBuffer, -1, sizeof(pixelBuffer));
	while (!quit)
	{
		handleEvents(&offset, &zoom, &MaxIterations, &K, &update, &quit);
		if (update)
		{
			memset(pixelBuffer, -1, sizeof(pixelBuffer));

			MinIterations = 0;
			MaxIterations = MinIterations + interval;

			update = false;
		}

		unsigned iTile = 0;

		for (auto &t : threads)
		{
			if (iTile >= tiles.size()) break;
			t = std::thread(renderManPart, MinIterations, MaxIterations, zoom, offset, tiles[iTile]);
			iTile++;
		}

		for (auto &t : threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}


		//Clear screen
		/*SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(gRenderer);

		for (unsigned i = 0; i < tiles.size(); i++)
		{
			threads[i] = std::thread(renderPart, tiles[i]);	
		}

		for (auto &t : threads)
		{
			if (t.joinable()) t.join();
		}*/

		std::thread render(wholeRender);
		render.join();

		
		text.printText(offset, zoom, MaxIterations);
		//Update screen
		SDL_RenderPresent(gRenderer);

		//MinIterations += interval;
		MaxIterations += interval;
	}
	close();
}