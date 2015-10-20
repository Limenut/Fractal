#pragma once

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Complex.h"
//#include "Globals.h"

class textOverlay
{
public:
	textOverlay();
	SDL_Surface *surface = nullptr;
	SDL_Rect rc[4];
	std::string str[4];
	SDL_Color color;
	TTF_Font *font;
	SDL_Renderer *ren;
	int fontHeight;

	void initText(SDL_Renderer* _ren, TTF_Font* _font);
	void update();
	SDL_Surface* printText(complex offset, double zoom, unsigned MaxIterations);
};