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
	static Word& CombineToWord(Byte const& a, Byte const& b)
	{
		Word w = ((a << 8) | b);
		return w;
	}
	
	//Bit setting/getting
	static void SetByteBits(Byte& var, Byte const& value, Byte const& pos, Byte const& len)
	{
		var = (var&~(len << pos)) | (value&len) << pos;
	}
	static Byte GetByteBits(Byte const& var, Byte const& pos, Byte const& len)
	{
		return ((var >> ((pos + 1) - len))&len);
	}
	static void SetByteBit(Byte& var, Byte const& value, Byte const& pos) 
	{
		SetByteBits(var, value, pos, 1);
	}
	static Byte GetByteBit(Byte const& var, Byte const& pos)
	{
		return (var >> pos & 1);
	}
	
	//Rotations
	static void RotateLeft(Byte& a, Byte const& n)
	{
		a = (a << n) | (a >> (8 - n));
	}
	static void RotateRight(Byte& a, Byte const& n)
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

