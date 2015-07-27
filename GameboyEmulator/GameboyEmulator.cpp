// GameboyEmulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SDL2\SDL.h>

#include "Memory.h"
#include "Cpu.h"
#include "Video.h"

#include <iostream>
#include <string>

SDL_Window* window;
SDL_Event event;
SDL_Renderer* renderer;
SDL_Texture* texture;
int quit;
int steps = 0;

const std::string gameDir = "../roms/";

int _tmain(int argc, _TCHAR* argv[])
{
	//Init components:
	Video::GetInstance();
	Cpu::GetInstance();
	Memory::GetInstance();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not init %s\n", SDL_GetError());
		return 0;
	}

	window = SDL_CreateWindow("Gameboy emulator", 500, 300, Video::cScreenWidth, Video::cScreenHeight, SDL_WINDOW_RESIZABLE);
	if (window == NULL)
	{
		printf("Unable to create window %s\n", SDL_GetError());
		return 0;
	}
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == NULL)
	{
		printf("Unable to create renderer %s\n", SDL_GetError());
		return 0;
	}
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, Video::cScreenWidth, Video::cScreenHeight);
	Uint32 pixels[Video::cScreenWidth*Video::cScreenHeight];
	memset(pixels, 255, Video::cScreenWidth*Video::cScreenHeight*sizeof(Uint32));



	//SDL init success - start emulator
	if (Memory::GetInstance().LoadGame(gameDir + "super_mario_land.gb"))
	{
		Memory::GetInstance().Reset();
		Cpu::GetInstance().Reset();
		Video::GetInstance().Reset();
		while (!quit)
		{
			Cpu::GetInstance().Execute();
			Video::GetInstance().Step();

			if (Video::GetInstance().justDrew)
			{
				Video::GetInstance().UpdatePixels(pixels);
				SDL_UpdateTexture(texture, NULL, pixels, Video::cScreenWidth * sizeof(Uint32));
			}

			SDL_PollEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;

			}
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			if (Cpu::GetInstance().HasError())
			{
				quit = true;
			}
		}
	}


	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}


