#pragma once

#include "Globals.h"
#include "Complex.h"
#include <SDL2/SDL.h>
#include <iostream>

class FractalRenderer
{
protected:
	complex minVal;
	complex factorVal;
	virtual int math(int x, int y, unsigned maxIterations) = 0;
	double zoom;
	complex offset;

public:
	void render(unsigned iTile, unsigned maxIterations);
	
};

class MandelRenderer : public FractalRenderer
{
public:
	MandelRenderer();
	int math(int x, int y, unsigned maxIterations);
	double centerOff;
	void update(double _zoom, complex _offset);
};

class JuliaRenderer : public FractalRenderer
{
public:
	JuliaRenderer();
	int math(int x, int y, unsigned maxIterations);
	complex K;
	void update(double _zoom, complex _offset, complex _K);
};

JuliaRenderer::JuliaRenderer()
{
	minVal = { -1.5, -1.5 };
	factorVal = { 3.0 / (SCREEN_WIDTH - 1), 3.0 / (SCREEN_HEIGHT - 1) };
	zoom = 1;
	offset = {0,0};
}

MandelRenderer::MandelRenderer()
{
	minVal = { -2.0, -1.5 };
	factorVal = { 3.0 / (SCREEN_WIDTH - 1), 3.0 / (SCREEN_HEIGHT - 1) };
	centerOff = 0.5;
	zoom = 1;
	offset = { 0,0 };
}

void MandelRenderer::update(double _zoom, complex _offset)
{
	zoom = _zoom;
	offset = _offset;
}

void JuliaRenderer::update(double _zoom, complex _offset, complex _K)
{
	zoom = _zoom;
	offset = _offset;
	K = _K;
}

void FractalRenderer::render(unsigned iTile, unsigned maxIterations)
{
	SDL_Rect tile = tiles[iTile];
	SDL_Surface *surf = surfaces[iTile];

	SDL_LockSurface(surf);

	Uint32 *pixels = (Uint32 *)surf->pixels;

	for (int y = tiles[iTile].y; y < tile.y + tile.h; ++y)
	{
		for (int x = tile.x; x < tile.x + tile.w; ++x)
		{
			int locX = x - tile.x;
			int locY = y - tile.y;

			if (pixels[(locY * surf->w) + locX] & 0x00FFFFFF) continue; //if the pixel is already colored

			int iterations = math(x, y, maxIterations);

			if (iterations >= 0)
			{
				int hue = (iterations * 12) % (256 * 6);

				//Set the pixel
				pixels[(locY * surf->w) + locX] = color_table[hue];
			}
		}
	}
	SDL_UnlockSurface(surf);
}

int MandelRenderer::math(int x, int y, unsigned maxIterations)
{
	complex c;
	c.im = (minVal.im + y * factorVal.im) * zoom + offset.im;
	c.re = (minVal.re + centerOff + x * factorVal.re) * zoom + offset.re - centerOff;

	//cardioid-check
	double cardioid = (c.re - 0.25)*(c.re - 0.25) + c.im*c.im;
	if (cardioid*(cardioid + (c.re - 0.25)) < 0.25*c.im*c.im)
		return -1;

	//first sphere-check
	if ((c.re + 1.0)*(c.re + 1.0) + c.im*c.im < (1.0 / 16.0))
		return -1;

	complex Z;
	Z.re = c.re, Z.im = c.im;
	unsigned iterations;

	//escape time algorithm
	for (iterations = 0; iterations < maxIterations; ++iterations)
	{
		complex Z_2;
		Z_2.re = Z.re*Z.re, Z_2.im = Z.im*Z.im;

		if (Z_2.re + Z_2.im > 4)
		{
			return iterations;
		}

		Z.im = 2 * Z.re*Z.im + c.im;
		Z.re = Z_2.re - Z_2.im + c.re;
	}
	return -1;
}

int JuliaRenderer::math(int x, int y, unsigned maxIterations)
{ 
	complex Z;
	Z.re = (minVal.re + x * factorVal.re) * zoom + offset.re;
	Z.im = (minVal.im + y * factorVal.im) * zoom + offset.im;
	unsigned iterations;

	for (iterations = 0; iterations < maxIterations; ++iterations)
	{
		complex Z_2;
		Z_2.re = Z.re*Z.re, Z_2.im = Z.im*Z.im;


		if (Z_2.re + Z_2.im > 4)
		{
			return iterations;
		}
		Z.im = 2 * Z.re*Z.im + K.im;
		Z.re = Z_2.re - Z_2.im + K.re;
	}
	return -1;
}