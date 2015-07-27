#pragma once
#include "GameboyHelper.h"

#include <string>
#include <vector>

class Memory
{
public:
	
	inline static Memory& GetInstance()
	{
		static Memory instance;
		return instance;
	}
	bool LoadGame(std::string gamePath);
	void Reset();

	//Read/write
	const Byte ReadByte(Word const& address);
	const Word ReadWord(Word const& address);
	void WriteByte(Byte const& value, Word const& address);
	void WriteWord(Word const& value, Word const& address);

	//Interrupts
	void SetInterruptEnable(Word value)
	{
		IE |= value;
	}
	void SetInterruptFlag(Word value)
	{
		IF |= value;
	}

	Word const& GetInterruptEnable()
	{
		return IE;
	}
	Word const& GetInterruptFlag()
	{
		return IF;
	}
	
private:
	Memory(){}
	~Memory();
	Byte* rom;
	Byte workRam[0x2000];
	Byte extRam[0x2000];
	Byte highRam[0x7F];
	Word IE;
	Word IF;

	//Catridge type
	typedef int MBC_mode;
	enum{ None, MBC1, MBC2, MBC3 };
	
	typedef int MaxMemMode;
	enum{ rom16M_ram8K, rom4M_ram32K };
	
	typedef struct
	{
		MBC_mode mode;
		unsigned int romSize;
		unsigned int romBanks;
		Byte romBank;
		unsigned int ramSize;
		unsigned int ramBanks;
		Byte ramBank;
		unsigned int hasRam;
		unsigned int hasBatt;
		unsigned int isRamOn;
		MaxMemMode maxMemoryMmode;
	}CartInfo;
	CartInfo cartInfo;
	void SetCartridgeInfo();
	
	bool isDebugging = true;

	//For cartridges larger than 32k
	Byte romOffset;
	Byte ramOffset;

	void MBC1_WriteByte(Byte const& value, Word const& address);
	void MBC2_WriteByte(Byte const& value, Word const& address);
	void MBC3_WriteByte(Byte const& value, Word const& address);
};

