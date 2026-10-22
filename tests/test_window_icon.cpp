// The Linux client's compiled-in window icon (AppIcon.h, ADR 0030) decodes with SDL's PNG loader at 128 x 128.
// A regenerated icon SDL can't read, or broken AppIcon.h generation, would otherwise show only as a log line.
#include "AppIcon.h"

#include <SDL3/SDL.h>

#include <stdio.h>

int main()
{
	SDL_Surface* icon = SDL_LoadPNG_IO(SDL_IOFromConstMem(g_appIconPng, sizeof(g_appIconPng)), true);
	if (!icon)
	{
		printf("FAIL: the built-in window icon doesn't decode: %s\n", SDL_GetError());
		return 1;
	}
	int w = icon->w, h = icon->h;
	SDL_DestroySurface(icon);
	if (w != 128 || h != 128)
	{
		printf("FAIL: the built-in window icon is %d x %d, expected 128 x 128\n", w, h);
		return 1;
	}
	printf("ok: built-in window icon decodes at 128 x 128\n");
	return 0;
}
