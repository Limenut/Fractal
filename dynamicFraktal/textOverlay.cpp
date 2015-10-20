#include "textOverlay.h"
#include <sstream>

textOverlay::textOverlay() {}

void textOverlay::initText(SDL_Renderer* _ren, TTF_Font* _font)
{
	font = _font;
	ren = _ren;

	fontHeight = TTF_FontLineSkip(font);

	for (int i = 0; i < 4; i++) { str[i] = " "; }
	color = { 0xFF, 0xFF, 0xFF };
	
	for (int i = 0; i < 4; i++)
	{
		rc[i].x = 0, rc[i].y = i * 20;
	}

	update();
}

void textOverlay::update()
{
	int width = 0;
	int height = 0;

	SDL_Surface* surf[4];
	for (int i = 0; i < 4; i++)
	{
		surf[i] = TTF_RenderText_Solid(font, str[i].c_str(), color);

		if (surf[i]->w > width) width = surf[i]->w;
		height += 2*fontHeight;
	}

	SDL_FreeSurface(surface);
	surface = SDL_CreateRGBSurface(0, width, height, 32, 
		0xff000000,
		0x00ff0000,
		0x0000ff00,
		0x000000ff);

	SDL_FillRect(surface, NULL, 0);

	for (int i = 0; i < 4; i++)
	{
		SDL_BlitSurface(surf[i], NULL, surface, &rc[i]);
		//surface pois
		SDL_FreeSurface(surf[i]);
	}
}

SDL_Surface* textOverlay::printText(complex offset, double zoom, unsigned MaxIterations)
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

	update();

	return surface;
}