#pragma once

#include "Complex.h"
#include <SDL2/SDL.h>

Uint32 color_table[256];

class FractalRenderer
{
protected:
	complex minVal;
	complex factorVal;
	int math(int x, int y);

public:
	unsigned MinIterations;
	unsigned MaxIterations;
	double zoom;
	complex offset;
	SDL_Rect tile;
	SDL_Surface* surface;
	void render();
};

class MandelRenderer : FractalRenderer
{
	int math(int x, int y);
	int centerOff;
};

class JuliaRenderer : FractalRenderer
{
	int math(int x, int y);
	complex K;
};

void FractalRenderer::render()
{

	SDL_LockSurface(surface);

	Uint32 *pixels = (Uint32 *)surface->pixels;

	for (int y = tile.y; y < tile.y + tile.h; ++y)
	{
		for (int x = tile.x; x < tile.x + tile.w; ++x)
		{
			int locX = x - tile.x;
			int locY = y - tile.y;

			if (pixels[(locY * surface->w) + locX] & 0x00FFFFFF) continue; //if the pixel is already colored

			int iterations = math(x,y);

			if (iterations >= 0)
			{
				int hue = (iterations * 12) % (256 * 6);

				//Set the pixel
				pixels[(locY * surface->w) + locX] = color_table[hue];
			}
		}
	}
	SDL_UnlockSurface(surface);
}

int MandelRenderer::math(int x, int y)
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
	for (iterations = MinIterations; iterations < MaxIterations; ++iterations)
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

int JuliaRenderer::math(int x, int y)
{
	complex Z;
	Z.re = (minVal.re + x * factorVal.re) * zoom + offset.re;
	Z.im = (minVal.im + y * factorVal.im) * zoom + offset.im;
	unsigned iterations;

	for (iterations = MinIterations; iterations < MaxIterations; ++iterations)
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