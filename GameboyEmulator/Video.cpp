#include "stdafx.h"
#include "Video.h"
#include "Cpu.h"

#include <iostream>
#include <fstream>

const Byte nintendo_intro[48] =
{
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0xC0, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E

};
void Video::Reset()
{
	int x, y, tile, obj;
	for (y = 0; y < cScreenHeight; y++)
	{
		for (x = 0; x < cScreenWidth; x++)
		{
			SetPixel(x, y, White);
		}
	}
	for (tile = 0; tile < 384; tile++)
	{
		for (y = 0; y < 8; y++)
		{
			for (x = 0; x < 8; x++)
			{
				bg_tiles[tile][y][x] = White;
			}
		}
	}

	for (obj = 0; obj < 40; obj++)
	{
		objects[obj].x = 0;
		objects[obj].y = 0;
		objects[obj].patternNum = 0;
		objects[obj].priority = 0;
		objects[obj].yFlip = 0;
		objects[obj].xFlip = 0;
		objects[obj].palNum = 0;

		oam[obj + 0] = 0;
		oam[obj + 1] = 0;
		oam[obj + 2] = 0;
		oam[obj + 3] = 0;
	}
	mode = 0;
	clock = 0;
	for (int i = 0; i < 0x30; i++)
	{
		WriteByte(nintendo_intro[i], 0x8000 + i);
	}
}

//IO
Byte Video::ReadByte(Word address)
{
	Byte b = 0x0;
	if (address < 0xA000)
	{
		if (address > 0x8FFF){ std::cout << "Reading from bg tile" << std::endl; }
		else{ std::cout << "Reading from spr/bg tiles" << std::endl; }

		return vram[address & 0x1FFF];
	}
	else if (address >= 0xFE00 && address < 0xFEA0)
	{
		std::cout << "Reading from oam" << std::endl;
		return oam[address & 0xFE9F];
	}
	else
	{
		switch (address & 0x00FF)
		{
		case 0x40:return lcdc;
		case 0x41:return lcdc_stat;
		case 0x42:return scroll_y;
		case 0x43:return scroll_x;
		case 0x44:return vert_line;
		case 0x45:return vert_line_cp;
		case 0xFA:return window_x;
		case 0xFB:return window_y;
		case 0x47:return bg_palette;
		case 0x48:return obj_palette0;
		case 0x49:return obj_palette1;
		}
	}
	return b;
}
void Video::WriteByte(Byte value, Word address)
{
	if (address < 0x9800)//Tile data
	{
		vram[address & 0x1FFF] = value;
		if (address > 0x8FFF){ if (isDebugging) std::cout << "Writing to bg tile data" << std::endl;; }
		else{ if (isDebugging)std::cout << "Writing to spr/bg/win tile data" << std::endl;; }

		UpdateTileData(value, address);
	}
	else if (address < 0xA000)//Tile map
	{
		if (isDebugging) std::cout << (address & 0x1FFF) << "Bg tile map, updating to use tile " << value << " from " << vram[address & 0x1FFF] << std::endl;
		vram[address & 0x1FFF] = value;
	}
	else if (address >= 0xFE00 && address < 0xFEA0)
	{
		if (isDebugging) std::cout << "Writing to oam" << std::endl;
		oam[address & 0xFE9F] = value;
		UpdateObject(value, address);
	}
	else
	{
		if (isDebugging) std::cout << "Writing to flag:" << GameBoyHelper::HexFormatOutX(address) << std::endl;
		switch (address)
		{
		case 0xFF40: lcdc = value; break;
		case 0xFF41: lcdc_stat = value; break;
		case 0xFF42: scroll_y = value; break;
		case 0xFF43: scroll_x = value; break;
		case 0xFF44: vert_line = 0; break;
		case 0xFF45: vert_line_cp = value; break;
		case 0xFF4A: window_y = value; break;
		case 0xFF4B: window_x = value; break;
		case 0xFF47: bg_palette = value; UpdatePalette(backgroundPalette, value); break;
		case 0xFF48: obj_palette0 = value; UpdatePalette(objectPalette0, value); break;
		case 0xFF49: obj_palette1 = value; UpdatePalette(objectPalette1, value); break;
		case 0xFF46:
		{
			Word baseAdrs = value;
			for (int i = 0x0; i < 0xF9; i++)
			{
				Byte val = Memory::GetInstance().ReadByte(baseAdrs + i);
				oam[i] = val;
				UpdateObject(val, baseAdrs+i);
			}
			break;
		}
		}
	}
}
void Video::UpdateTileData(Byte value, Word address)
{

	Word writeAddress = address;
	writeAddress &= 0x1FFF;
	if (address & 1){ writeAddress--; } //Because each line is represented as 2 lines, start with the first one
	int tile = (writeAddress >> 4); // shift 4=div 16 - Each tile is 16 byte - 256x2 tiles

	if (isDebugging) std::cout << "Updating tile:" << tile << std::endl;

	int y = (writeAddress >> 1) & 7;	//
	int sx;
	int bitValue1 = 0;
	int bitValue2 = 0;
	for (int x = 0; x < 8; x++)
	{
		sx = 1 << (7 - x);
		bitValue1 = (vram[writeAddress] & sx) ? 1 : 0;
		bitValue2 = (vram[writeAddress + 1] & sx) ? 2 : 0;
		bg_tiles[tile][y][x] = bitValue1 + bitValue2;
	}
}
void Video::UpdateObject(Byte value, Word address)
{
	int obj = (address & 0x00FF) >> 2;//Get F9 bits, divide with 4 to get obj correct id
	std::cout << "Updating object: " << obj << std::endl;
	switch (address & 3)
	{
	case 0:
		objects[obj].y = value;
		break;
	case 1:
		objects[obj].x = value;
		break;
	case 2:
		objects[obj].patternNum = value;
		break;
	case 3:
		objects[obj].priority = GameBoyHelper::GetByteBit(value, 7);
		objects[obj].yFlip = GameBoyHelper::GetByteBit(value, 6);
		objects[obj].xFlip = GameBoyHelper::GetByteBit(value, 5);
		objects[obj].palNum = GameBoyHelper::GetByteBit(value, 4);
		break;
	}
}
void Video::UpdatePalette(Color palette[], Byte value)
{
	for (int i = 0; i < 4; i++)
	{
		switch ((value >> (i * 2)) & 3)
		{
		case 0: palette[i] = Black; break;
		case 1: palette[i] = Dark_Gray; break;
		case 2: palette[i] = Light_Gray; break;
		case 3: palette[i] = White; break;
		}
	}
}

//Updating state and drawing
void Video::Step()
{
	clock += Cpu::GetInstance().GetClockT();
	if (vert_line == vert_line_cp)
	{
		GameBoyHelper::SetByteBit(lcdc_stat, 1, 2);
		if (GameBoyHelper::GetByteBit(lcdc_stat, 6) == 1)
		{
			Memory::GetInstance().SetInterruptFlag(0x2);
		}
	}
	else
	{
		GameBoyHelper::SetByteBit(lcdc_stat, 0, 2);
	}
	switch (mode)
	{
		//OAM read
	case 2:
		if (clock >= 80)
		{
			clock = 0;
			mode = 3;
		}
		break;

		//OAM and VRAM reading
	case 3:
		if (clock >= 172)
		{
			clock = 0;
			mode = 0;
			RenderScan();
			if (GameBoyHelper::GetByteBit(lcdc_stat, 3) == 1)
			{
				Memory::GetInstance().SetInterruptFlag(0x2);
			}
		}
		break;

		//HBlank
	case 0:
		if (clock >= 204)
		{
			clock = 0;
			vert_line++;
			if (vert_line == 143)
			{
				mode = 1;
				justDrew = true;
				if (GameBoyHelper::GetByteBit(lcdc_stat, 4) == 1)
				{
					Memory::GetInstance().SetInterruptFlag(0x2);
				}
				Memory::GetInstance().SetInterruptFlag(0x1);
				WriteTileDataToFile("../tiledata.txt");
				WriteTileMapToFile("../tilemap.txt");
			}
			else
			{
				mode = 2;
				if (GameBoyHelper::GetByteBit(lcdc_stat, 5) == 1)
				{
					Memory::GetInstance().SetInterruptFlag(0x2);
				}
			}
		}
		break;

		//VBlank
	case 1:
		justDrew = false;
		if (clock >= 456)
		{
			vert_line++;
			clock = 0;
			if (vert_line > 153)
			{
				mode = 2;
				vert_line = 0;
			}
		}
		break;
	default:printf("Unknown mode %d", mode);
	}
}
void Video::RenderScan()
{
	if (GameBoyHelper::GetByteBit(lcdc, 0))//Display BG and window?
	{
		// VRAM offset for the tile map
		int mapoffs = GameBoyHelper::GetByteBit(lcdc, 3) ? 0x1C00 : 0x1800;

		// Which line of tiles to use in the map
		mapoffs += ((vert_line + scroll_y) & 255) >> 3;

		// Which tile to start with in the map line
		int lineoffs = (scroll_x >> 3);

		// Which line of pixels to use in the tiles
		int y = (vert_line + scroll_y) & 7;
		// Where in the tileline to start
		int x = scroll_x & 7;

		// Where to render on the canvas
		int canvasoffs = vert_line * cScreenWidth;

		// Read tile index from the background map
		int tile = vram[mapoffs + lineoffs];

		// If the tile data set in use is #1, the
		// indices are signed; calculate a real tile offset
		if (GameBoyHelper::GetByteBit(lcdc, 4) == 1 && tile < 128) tile += 256;
		int i;
		for (i = 0; i < cScreenWidth; i++)
		{
			// Re-map the tile pixel through the palette
			Color palColor = backgroundPalette[bg_tiles[tile][y][x]];

			// Plot the pixel to canvas
			screenBuffer[canvasoffs] = palColor;
			canvasoffs++;

			// When this tile ends, read another
			x++;
			if (x == 8)
			{
				x = 0;
				lineoffs = (lineoffs + 1) & 31;
				tile = vram[mapoffs + lineoffs];
				if (GameBoyHelper::GetByteBit(lcdc, 4) == 1 && tile < 128) tile += 256;
			}
		}
	}

}
void Video::UpdatePixels(unsigned int(&pixels)[cScreenWidth*cScreenHeight])
{
	unsigned int color = 0xFF00FF00;
	for (int i = 0; i < cScreenWidth*cScreenHeight; i++)
	{
		switch (screenBuffer[i])
		{
		case White:color = 0xFF9CBD0F; break;
		case Light_Gray:color = 0xFF8CAD0F; break;
		case Dark_Gray:color = 0xFF306230; break;
		case Black:color = 0xFF0F380F; break;

		}
		pixels[i] = color;

	}
}

void Video::WriteTileDataToFile(std::string file)
{
	std::fstream f(file, std::ios::out | std::ios::app | std::ofstream::trunc);
	if (f.is_open())
	{
		for (int i = 0; i < 384; i++)
		{
			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					f << std::to_string(bg_tiles[i][y][x]);
				}
				f << "\n";
			}
			f << "\n";
		}
		f.close();
	}
}
void Video::WriteTileMapToFile(std::string file)
{
	std::fstream f(file, std::ios::out | std::ios::app | std::ofstream::trunc);
	if (f.is_open())
	{

		int mapoffs = GameBoyHelper::GetByteBit(lcdc, 3) ? 0x1C00 : 0x1800;
		for (int i = 0; i < 0x3FF; i++)
		{
			f << std::to_string(vram[mapoffs + i]);
			if ((i + 1) % 16 == 0)
			{
				f << "\n";
			}
		}
		f.close();
	}
}