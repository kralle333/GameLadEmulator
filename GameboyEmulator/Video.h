#pragma once

#include "GameboyHelper.h"

class Video
{
public:

	//Variables
	static const int cScreenWidth = 160;
	static const int cScreenHeight = 144;
	bool justDrew = false;

	//Constructors
	inline static Video& GetInstance()
	{
		static Video instance;
		return instance;
	}
	void Reset();

	//Methods
	Byte ReadByte(Word address);
	void WriteByte(Byte const&, Word address);
	void UpdatePixels(unsigned int(&pixels)[cScreenWidth*cScreenHeight]);
	void Step();
private:
	//Typedefs
	typedef Byte Color;
	enum { White, Light_Gray, Dark_Gray, Black };
	typedef Color Tile16[8][8];
	typedef struct
	{
		int x;
		int y;
		int patternNum;
		int priority;
		int yFlip;
		int xFlip;
		int palNum;
	}ObjData;

	//Variables
	Color screenBuffer[Video::cScreenWidth *Video::cScreenHeight];
	ObjData objects[40];
	Byte vram[0x2000];
	Byte oam[0xA0];
	Tile16 bg_tiles[384];


	unsigned int clock;
	int mode;
	bool writeToFile = true;

	//Video registers
	Byte lcdc; //FF40
	Byte lcdc_stat;//FF41
	Byte scroll_x;//FF42
	Byte scroll_y;//FF43
	Byte vert_line;//FF44
	Byte vert_line_cp;//FF45
	Byte window_y;//FF4A
	Byte window_x;//FF4B
	Byte bg_palette;//FF47;
	Byte obj_palette0;//FF48
	Byte obj_palette1;//FF49

	//For easy handling of colors
	Color backgroundPalette[4];//FF47
	Color objectPalette0[4];//FF48
	Color objectPalette1[4];//FF49

	bool isDebugging = true;

	//Constructors
	Video(){}
	Video(Video const&) = delete;
	void operator=(Video const&) = delete;

	//Methods
	Color GetPixel(int x, int y){ return screenBuffer[x + (y*cScreenWidth)]; }
	void SetPixel(int x, int y, Color const color){ screenBuffer[x + (y*cScreenWidth)] = color; }
	void RenderScan();
	void UpdateTileData(Byte  value, Word address);
	void UpdateObject(Byte  value, Word  address);
	void UpdatePalette(Color palette[], Byte value);
	void PrintTile(int tile)
	{
		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 8; x++)
			{
				std::cout << std::to_string(bg_tiles[tile][y][x]);
			}
			std::cout << std::endl;
		}
	}
	void PrintScreen();
	void WriteTileDataToFile(std::string path);
	void WriteTileMapToFile(std::string path);
	void Test();
};

