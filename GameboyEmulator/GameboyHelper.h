#pragma once

#include <iostream>
#include <sstream>
#include <ctype.h>
#include <iomanip>

typedef unsigned char Byte;
typedef unsigned short Word;

class GameBoyHelper
{
public:

	static enum InterruptType
	{
		Vblank,LCDSTAT,Timer,Serial,Joypad
	};

	static std::string HexFormatOutX(int value)
	{
		std::stringstream stream;
		stream << std::hex << std::setw(2) << std::setfill('0') << value;
		std::string result(stream.str());
		for (int i = 0; i < result.length(); i++)
		{
			result[i] = toupper(result[i]);
		}
		return "0x"+result;
	}
	static std::string HexFormatOut(int value)
	{
		std::stringstream stream;
		stream << std::hex << std::setw(2) << std::setfill('0') << value;
		std::string result(stream.str());
		for (int i = 0; i < result.length(); i++)
		{
			result[i] = toupper(result[i]);
		}
		return result;
	}
	static Word& CombineToWord(Byte a, Byte b)
	{
		Word w = ((a << 8) | b);
		return w;
	}
	
	//Bit setting/getting
	static void SetByteBits(Byte& var, Byte value, Byte pos, Byte len)
	{
		var = (var&~(len << pos)) | (value&len) << pos;
	}
	static Byte GetByteBits(Byte var, Byte pos, Byte len)
	{
		return ((var >> ((pos + 1) - len))&len);
	}
	static void SetByteBit(Byte& var, Byte value, Byte pos) 
	{
		SetByteBits(var, value, pos, 1);
	}
	static Byte GetByteBit(Byte var, Byte pos)
	{
		return (var >> pos & 1);
	}
	
	//Rotations
	static void RotateLeft(Byte& a, Byte n)
	{
		a = (a << n) | (a >> (8 - n));
	}
	static void RotateRight(Byte& a, Byte n)
	{
		a = (a >> n) | (a << (8 - n));
	}
	static void SwapHexits(Byte& a)
	{
		RotateRight(a, 4);
	}
private:
	GameBoyHelper();
};

