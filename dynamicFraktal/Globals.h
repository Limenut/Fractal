#pragma once

#include <vector>
#include <thread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

//global window and renderer
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font *gFont = NULL;

const Uint8* state = SDL_GetKeyboardState(NULL);

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 1024;

const unsigned THREAD_COUNT = 2*std::thread::hardware_concurrency();
const int TILE_SIZE = 256;

std::vector<SDL_Rect>tiles;
std::vector<SDL_Surface*>surfaces;
std::vector<std::thread>threads;

Uint32 color_table[256 * 6];
//std::vector<Uint32>color_table;