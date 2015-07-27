#pragma once
#include "GameBoyHelper.h"

#include <string>

class CpuHelper
{
public:
	static void SixteenBitSetRegs(const Word& value, Byte& reg1, Byte& reg2)
	{
		reg1 = ((value)& 0xFF00) >> 8;
		reg2 = (value)& 0x00FF;
	}
	static void Increase16bit(Byte& reg1, Byte& reg2)
	{
		SixteenBitSetRegs((GameBoyHelper::CombineToWord(reg1, reg2) + 1), reg1, reg2);
	}
	static void Decrease16bit(Byte& reg1, Byte& reg2)
	{
		SixteenBitSetRegs((GameBoyHelper::CombineToWord(reg1, reg2) - 1), reg1, reg2);
	}

	//Carrying
	static bool isHalfCarry(Byte v1, Byte v2) 
	{ 
		Word sum = (v1 & 0x0F) + (v2 & 0x0F);
		return sum & 0x10;
	}
	static bool isCarry(Byte v1, Byte v2) { return (0xFF - v2 < v1); }
	static bool  isHalfBorrow(Byte v1, Byte v2) 
	{
		return ((v1 & 0x0F) > (v2 & 0x0F));
	}
	static bool  isBorrow(Byte v1, Byte v2) { return (v1>v2); }
};

