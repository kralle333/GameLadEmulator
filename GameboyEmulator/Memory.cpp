#include "stdafx.h"
#include "Memory.h"
#include "Video.h"
#include "Cpu.h"

#include <fstream>
#include <iostream>

Memory::~Memory()
{
	delete[] rom;
}

bool Memory::LoadGame(std::string path)
{
	FILE *gameFile = 0;
	fopen_s(&gameFile, path.c_str(), "rb");
	if (gameFile != NULL)
	{
		fseek(gameFile, 0L, SEEK_END);
		long gameSize = ftell(gameFile);
		rewind(gameFile);
		if (gameSize == 0)
		{
			if (isDebugging){ std::cout << "Gamesize is 0 - Aborting" << std::endl; };
		}

		rom = new Byte[gameSize];
		if (rom == NULL)
		{
			if (isDebugging){ std::cout << "Allocation failure - Aborting" << std::endl; };
			fclose(gameFile);
			return 0;
		}

		fread(rom, 1, gameSize, gameFile);
		if (isDebugging){ std::cout << "Succesfully loaded game at: " << path << " with gamesize: " << gameSize << std::endl; };
		SetCartridgeInfo();
		fclose(gameFile);

		return 1;
	}
	else
	{
		if (isDebugging){ std::cout << "Could not load game at " << path << std::endl; };
	}
	return 0;
}
void Memory::SetCartridgeInfo()
{
	std::string cartType = "";
	switch (rom[0x0147])
	{
	case 0:cartInfo.mode = None; cartType = "None"; break;
	case 1:
	case 2:
	case 3:cartInfo.mode = MBC1; cartType = "MBC1"; break;
	case 5:
	case 6:cartInfo.mode = MBC2; cartType = "MBC2"; break;
	case 12:
	case 13:cartInfo.mode = MBC3; cartType = "MBC3"; break;
	default: printf("Unknown cart mode %d\n", rom[0x0147]);
	}
	switch (rom[0x0148])
	{
	case 0:cartInfo.romSize = 32; cartInfo.romBanks = 2; break;
	case 1:cartInfo.romSize = 64; cartInfo.romBanks = 4; break;
	case 2:cartInfo.romSize = 128; cartInfo.romBanks = 8; break;
	case 3:cartInfo.romSize = 256; cartInfo.romBanks = 16; break;
	case 4:cartInfo.romSize = 512; cartInfo.romBanks = 32; break;
	case 5:cartInfo.romSize = 1024; cartInfo.romBanks = 64; break;
	case 6:cartInfo.romSize = 2 * 1024; cartInfo.romBanks = 128; break;
	default: printf("Unknown rom size %d\n", rom[0x0148]);
	}
	switch (rom[0x0149])
	{
	case 0:cartInfo.ramSize = 0;   cartInfo.ramBanks = 0; break;
	case 1:cartInfo.ramSize = 2;   cartInfo.ramBanks = 1; break;
	case 2:cartInfo.ramSize = 8;   cartInfo.ramBanks = 1; break;
	case 3:cartInfo.ramSize = 32;  cartInfo.ramBanks = 4; break;
	case 4:cartInfo.ramSize = 128; cartInfo.ramBanks = 16; break;
	default: printf("Unknown ram size %d\n", rom[0x0149]);
	}
	romOffset = 0x4000;
	if (isDebugging){ std::cout << "CARTRIDGE TYPE:" << cartType << "- "; }
	if (isDebugging){ std::cout << "ROM SIZE: " << cartInfo.romSize << " kB- "; }
	if (isDebugging){ std::cout << "RAM SIZE : " << cartInfo.ramSize << "kB" << std::endl; };
}

void Memory::Reset()
{
	WriteByte(0x00, 0xFF05);//TIMA
	WriteByte(0x00, 0xFF06);//TMA
	WriteByte(0x00, 0xFF07);//TAC
	WriteByte(0x80, 0xFF10);//NR10
	WriteByte(0xBF, 0xFF11);//NR11
	WriteByte(0xF3, 0xFF12);//NR12
	WriteByte(0xBF, 0xFF14);//NR14
	WriteByte(0x3F, 0xFF16);//NR21
	WriteByte(0x00, 0xFF17);//NR22
	WriteByte(0xBF, 0xFF19);//NR24
	WriteByte(0x7F, 0xFF1A);//NR30
	WriteByte(0xFF, 0xFF1B);//NR31
	WriteByte(0x9F, 0xFF1C);//NR32
	WriteByte(0xBF, 0xFF1E);//NR33
	WriteByte(0xFF, 0xFF20);//NR41
	WriteByte(0x00, 0xFF21);//NR42
	WriteByte(0x00, 0xFF22);//NR43
	WriteByte(0xBF, 0xFF23);//NR30
	WriteByte(0x77, 0xFF24);//NR50
	WriteByte(0xF3, 0xFF25);//NR51
	WriteByte(0xF1, 0xFF26);
	WriteByte(0x91, 0xFF40);//LCDC
	WriteByte(0x00, 0xFF42);//SCY
	WriteByte(0x00, 0xFF43);//SCX
	WriteByte(0x00, 0xFF45);//LYC
	WriteByte(0xFC, 0xFF47);//BGP
	WriteByte(0xFF, 0xFF48);//OBP0
	WriteByte(0xFF, 0xFF49);//OBP1
	WriteByte(0x00, 0xFF4A);//WY
	WriteByte(0x00, 0xFF4B);//WX
	WriteByte(0x00, 0xFFFF);//IE
	WriteByte(0xE1, 0xFF0F);//IF
}

const Byte Memory::ReadByte(Word address)
{
	if (address < 0x4000)
	{
		return rom[address];
	}
	else if (address < 0x8000)
	{
		return rom[romOffset + (address & 0x3FFF)];
	}
	else if (address < 0xA000)
	{
		if (isDebugging){ std::cout << "Reading from Video" << std::endl; };
		return Video::GetInstance().ReadByte(address);
	}
	else if (address < 0xC000)
	{
		if (cartInfo.isRamOn && cartInfo.ramSize > 0)
		{
			if (isDebugging){ std::cout << "Reading from external ram\n" << std::endl; };
			return extRam[address & 0x1FFF];
		}
		else
		{
			if (isDebugging){ std::cout << "Reading from non existing ram\n" << std::endl; };
		}
	}
	else if (address < 0xE000)//D000-DFFF: 4KB work ram bank 0+1
	{
		if (isDebugging){ std::cout << "Reading from work ram\n" << std::endl; };
		return workRam[address & 0x1FFF];
	}
	else if (address < 0xFE00)//E000-FDFF: ECHO to work ram bank 0
	{
		if (isDebugging){ std::cout << "Reading from work ram ECHO\n" << std::endl; };
		return workRam[(address - 0x2000) & 0x1FFF];
	}
	else if (address >= 0xFF10 && address <= 0xFF30)
	{
		if (isDebugging){ std::cout << "Attempting to read from unimplemented sound flag at:" << address << std::endl; };
		return 0;
	}
	else if (address >= 0xFF41 && address < 0xFF4C)
	{
		if (isDebugging){ std::cout << "Reading from video flag at: " << address << std::endl; };
		return Video::GetInstance().ReadByte(address);
	}
	else if (address < 0xFFFF)
	{
		if (isDebugging){ std::cout << "Reading from high ram" << std::endl; };
		return highRam[address & 0x7F];
	}
	else if (address == 0xFFFF)
	{
		return IE;
	}
	if (isDebugging){ std::cout << "Unknown read!" << std::endl; };
	std::cin;

	return 0x0;
}
const Word Memory::ReadWord(Word address)
{
	Word w = GameBoyHelper::CombineToWord(ReadByte(address + 1), ReadByte(address));
	return w;
}

void Memory::WriteByte(Byte value, Word address)
{
	if (address < 0x8000)
	{
		if (isDebugging){ std::cout << "Writing to Rom" << std::endl; };
		switch (cartInfo.mode)
		{
		case MBC1:MBC1_WriteByte(value, address); break;
		case MBC2:MBC2_WriteByte(value, address); break;
		case MBC3:MBC3_WriteByte(value, address); break;
		default:printf("ERROR: writing to rom using cart type: %d\n", cartInfo.mode); break;
			break;
		}
	}
	else if (address < 0xA000)//8000-9FFF: VRAM
	{
		if (isDebugging){ std::cout << "Writing to VRAM" << std::endl; };
		Video::GetInstance().WriteByte(value, address);
	}
	else if (address < 0xC000)//A000-BFFF: Ram banks 0-3, if any
	{
		if (cartInfo.isRamOn && cartInfo.ramSize > 0)
		{
			if (isDebugging){ std::cout << "Writing to ext ram" << std::endl; };
			extRam[address & 0x1fff] = value;
		}
		else if (!cartInfo.isRamOn)
		{
			/*if(DEBUG_MEM)printf("Tried to write to unabled ram at 0x%X\n",address);
			D_BREAK();*/
		}
		else if (cartInfo.ramSize == 0)
		{
			/*if(DEBUG_MEM)printf("Tried to write to non-existent extern ram at 0x%X\n",address);
			D_BREAK();*/
		}
	}
	else if (address < 0xE000)//C000-DFFF: 4KB work ram bank 0+1
	{
		if (isDebugging){ std::cout << "Writing to workRam" << std::endl; };
		workRam[(address & 0x1FFF)] = value;
	}
	else if (address < 0xFE00)//E000-FE00: ECHO to work ram bank 0
	{
		if (isDebugging){ std::cout << "Writing to workRam ECHO" << std::endl; };
		workRam[(address - 0x2000) & 0x1FFF] = value;
	}
	else if (address < 0xFEA0)//FE00-FE9F: Sprite attribute table - OAM
	{
		if (isDebugging){ std::cout << "Writing to sprite attribute table NYI" << std::endl; };
		Video::GetInstance().WriteByte(value, address);
	}
	else if (address < 0xFF00)//FEA0-FEFF: Not usable
	{
		if (isDebugging){ std::cout << "Not usable...so why are you writing here?" << std::endl; };
	}
	else if (address < 0xFF80)//FF00-FF7F: I/O registers
	{
		if (address < 0xFF10)
		{
			switch (address & 0x0F)
			{
			case 0x00:
				std::cout << "Writing to unimplemented joypad address" << std::endl;
				//joypad_write_byte(value, address);
				break;
			case 0x01:
			case 0x02:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				std::cout << "Writing to unimplemented timer address:" << GameBoyHelper::HexFormatOutX(address) << std::endl;
				break;
			case 0x0F:
				IF = value;
				break;
			}
		}
		if (address >= 0xFF10 && address <= 0xFF3F)
		{
			if (isDebugging){ std::cout << "Attempting to write to unimplemented sound flag" << std::endl; };
		}
		else if (address >= 0xFF40 && address <= 0xFF4B)
		{
			if (isDebugging){ std::cout << "Writing to video flags" << std::endl; };
			Video::GetInstance().WriteByte(value, address);
		}
		else
		{
			if (isDebugging){ std::cout << "Writing NOT YET IMPLEMENTED! Address: " << address << std::endl; };
		}
	}
	else if (address < 0xFFFF)//FF80-FFFE: High ram (HRAM)
	{
		if (isDebugging){ std::cout << "Writing to high ram" << std::endl; };
		highRam[address & 0x7F] = value;
	}
	else if (address == 0xFFFF)//FFFF: Interrupt Enable Register
	{
		if (isDebugging){ std::cout << "Writing to interrupt enable register" << std::endl; };
		IE = value;
	}
	else
	{
		if (isDebugging){ std::cout << "Something wrong, unknown address:" << address << std::endl; };
	}
}
void Memory::MBC1_WriteByte(Byte value, Word address)
{
	if (address < 0x2000)//Ram enable
	{
		if (value == 0x0A)
		{
			cartInfo.isRamOn = true;
			if (isDebugging){ std::cout << "Ram enabled" << std::endl; };
		}
		else
		{
			if (isDebugging){ std::cout << "Ram disabled" << std::endl; };
			cartInfo.isRamOn = false;
		}
	}
	else if (address < 0x4000)//Rom bank selection
	{
		cartInfo.romBank = (value & 00011111) << 2;
		cartInfo.romBank = cartInfo.romBank == 0 ? 1 : cartInfo.romBank;
		cartInfo.romBank = cartInfo.romBank == 20 ? 21 : cartInfo.romBank;
		cartInfo.romBank = cartInfo.romBank == 40 ? 41 : cartInfo.romBank;
		cartInfo.romBank = cartInfo.romBank == 60 ? 61 : cartInfo.romBank;
		romOffset = (cartInfo.romBank - 1) * 0x4000;
	}
	else if (address < 0x6000) //4000-5FFF: Rom/Ram bank set
	{
		switch (cartInfo.maxMemoryMmode)
		{
		case rom16M_ram8K:
			cartInfo.romBank = (value & 0x3);
			cartInfo.romBank = cartInfo.romBank == 0 ? 1 : cartInfo.romBank;
			cartInfo.romBank = cartInfo.romBank == 20 ? 21 : cartInfo.romBank;
			cartInfo.romBank = cartInfo.romBank == 40 ? 41 : cartInfo.romBank;
			cartInfo.romBank = cartInfo.romBank == 60 ? 61 : cartInfo.romBank;
			romOffset = (cartInfo.romBank - 1) * 0x4000;
			break;
		case rom4M_ram32K:
			if (cartInfo.isRamOn)
			{
				cartInfo.ramBank = (value & 0x3);
				if (isDebugging){ std::cout << "Ram bank set to " << cartInfo.ramBank << std::endl; };
			}
			else
			{
				if (isDebugging){ std::cout << "Ram was not enabled, cant select bank!" << std::endl; };
			}
			break;
		default:if (isDebugging){ std::cout << "Unknown max memory mode cartInfo.mm_mode: " << cartInfo.maxMemoryMmode << std::endl; }; break;
		}
	}
	else if (address < 0x8000) //6000-7FFF: Maximum memory mode set
	{
		switch (value)
		{
		case 0:cartInfo.maxMemoryMmode = rom16M_ram8K; break;
		case 1:cartInfo.maxMemoryMmode = rom4M_ram32K; break;
		default:
			if (isDebugging){ std::cout << "Unknown Maximum memory set command address: " << address << " 0x" << value << std::endl; }; break;
		}
	}
}
void Memory::MBC2_WriteByte(Byte value, Word address)
{

}
void Memory::MBC3_WriteByte(Byte value, Word address)
{

}

void Memory::WriteWord(Word value, Word address)
{
	WriteByte(0x00FF & value, address);
	WriteByte(value >> 8, address + 1);
}


