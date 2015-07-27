#include "stdafx.h"
#include "Cpu.h"
#include "CpuHelper.h"
#include "Memory.h"
#include "Video.h"

#include <iostream>
#include <fstream>

static const std::string GameBoyOpcodes[256] =
{
	//00
	"NOP", "LD BC,nn", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,n", "RLCA",
	"LD (nn),SP", "ADD HL,BC", "ADD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,n", "RRCA",
	//10
	"STOP", "LD DE,nn", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,n", "RLA",
	"JR n", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,n", "RRA",
	//20
	"JR NZ,n", "LD HL,nn", "LD (HL+),A", "INC HL", "INC H", "DEC H", "LD H,n", "DAA",
	"JR Z,n", "ADD HL,HL", "LD A,(HL+)", "DEC HL", "INC L", "DEC L", "LD L,n", "CPL",
	//30
	"JR NC,n", "LD SP,nn", "LD (HL-),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),n", "SCF",
	"JR C,n", "ADD HL,SP", "LD A,(HL-)", "DEC SP", "INC A", "DEC A", "LD A,n", "CCF",

	//40
	"LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,H", "LD B,L", "LD B,(HL)", "LD B,A",
	"LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",
	//50
	"LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,H", "LD D,L", "LD D,(HL)", "LD D,A",
	"LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",
	//60
	"LD H,B", "LD H,C", "LD H,D", "LD H,E", "LD H,H", "LD H,L", "LD H,(HL)", "LD H,A",
	"LD L,B", "LD L,C", "LD L,D", "LD L,E", "LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",
	//70
	"LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E", "LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A",
	"LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",

	//80
	"ADD A,B", "ADD A,C", "ADD A,D", "ADD A,E", "ADD A,H", "ADD A,L", "ADD A,(HL)", "ADD A,A",
	"ADC A,B", "ADC A,C", "ADC A,D", "ADC A,E", "ADC A,H", "ADC A,L", "ADC A,(HL)", "ADC A,A",

	//90
	"SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A",
	"SBC B", "SBC C", "SBC D", "SBC E", "SBC H", "SBC L", "SBC (HL)", "SBC A",
	//A0
	"AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A",
	"XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
	//B0
	"OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A",
	"CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",
	//C0
	"RET NZ", "POP BC", "JP NZ,nn", "JP nn", "CALL NZ,nn", "PUSH BC", "ADD A,n", "RST 00h",
	"RET Z", "RET", "JP Z,nn", "CB", "CALL Z,nn", "CALL nn", "ADC n", "RST 08h",
	//D0
	"RET NC", "POP DE", "JP NZ,nn", "Error", "CALL NC,nn", "PUSH DE", "SUB n", "RST 10h",
	"RET C", "RETI", "JP C,nn", "Error", "CALL C,nn", "Error", "SBC n", "RST 18h",
	//E0
	"LDH (0xFF00+n),A", "POP HL", "LD (C),A", "Error", "Error", "PUSH HL", "AND", "RST 20h",
	"LD ADD SP,n", "JP (HL)", "LD (nn),A", "Error", "Error", "Error", "XOR n", "RST 28h",
	//F0
	"LDH A,(0xFF00+n)", "POP AF", "LD A,(C)", "DI", "Error", "PUSH AF", "OR n", "RST 30h",
	"LD HL,SP+n", "LD SP,HL", "LD A,(nn)", "EI", "Error", "Error", "CP n", "RST 38h",
};

Cpu::Cpu()
{
	//Remove log file
	std::remove("../outputLog.txt");

	//4 top rows
	std::function<void()> f;

	for (int i = 0x0; i <= 0x3F; ++i)
	{
		switch (i & 0x0F)
		{
		case 0x00:f = (i < 0x20) ? std::bind(&Cpu::Misc, this) : std::bind(&Cpu::Jump, this); break;
		case 0x01:f = std::bind(&Cpu::SixteenBitLoad, this); break;
		case 0x02:f = std::bind(&Cpu::EightBitLoad, this); break;
		case 0x03:f = std::bind(&Cpu::SixteenBitInc, this); break;
		case 0x04:f = std::bind(&Cpu::EightBitInc, this); break;
		case 0x05:f = std::bind(&Cpu::EightBitDec, this); break;
		case 0x06:f = std::bind(&Cpu::EightBitLoad, this); break;
		case 0x07:f = (i < 0x20) ? std::bind(&Cpu::RotatesMisc, this) : std::bind(&Cpu::Misc, this); break;
		case 0x08:f = (i >= 0x10) ? std::bind(&Cpu::Jump, this) : std::bind(&Cpu::SixteenBitLoad, this); break;
		case 0x09:f = std::bind(&Cpu::SixteenBitAdd, this); break;
		case 0x0A:f = std::bind(&Cpu::EightBitLoad, this); break;
		case 0x0B:f = std::bind(&Cpu::SixteenBitDec, this); break;
		case 0x0C:f = std::bind(&Cpu::EightBitInc, this); break;
		case 0x0D:f = std::bind(&Cpu::EightBitDec, this); break;
		case 0x0E:f = std::bind(&Cpu::EightBitLoad, this); break;
		case 0x0F:f = (i < 0x20) ? std::bind(&Cpu::RotatesMisc, this) : std::bind(&Cpu::Misc, this); break;
		default:f = std::bind(&Cpu::Error, this); break;
		}
		functionMap[i] = f;
	}
	f = nullptr;

	//4 next rows: state.Eight bit loads (Except 0x76 = state.Halt)
	for (int i = 0x40; i <= 0x7F; ++i)
	{
		functionMap[i] = std::bind(&Cpu::EightBitLoad, this);
	}
	functionMap[0x76] = std::bind(&Cpu::Misc, this);

	//4 next rows: state.Eight bit Arithmetic/control
	for (int i = 0x80; i <= 0xBF; ++i)
	{
		switch (i & 0xF0)
		{
		case 0x80:f = ((i & 0x0F) <= 0x07) ? std::bind(&Cpu::EightBitAdd, this) : std::bind(&Cpu::EightBitAdc, this); break;
		case 0x90:f = ((i & 0x0F) <= 0x07) ? std::bind(&Cpu::EightBitSub, this) : std::bind(&Cpu::EightBitSbc, this); break;
		case 0xA0:f = ((i & 0x0F) <= 0x07) ? std::bind(&Cpu::EightBitAnd, this) : std::bind(&Cpu::EightBitXor, this); break;
		case 0xB0:f = ((i & 0x0F) <= 0x07) ? std::bind(&Cpu::EightBitOr, this) : std::bind(&Cpu::EightBitCp, this); break;
		default:f = std::bind(&Cpu::Error, this); break;
		}
		functionMap[i] = f;
	}
	f = nullptr;

	//4 next rows: Mixed
	for (int i = 0xC0; i <= 0xFF; ++i)
	{
		switch (i & 0x0F)
		{
		case 0x00:f = (i < 0xE0) ? std::bind(&Cpu::Return, this) : std::bind(&Cpu::EightBitLoad, this); break;
		case 0x01:f = std::bind(&Cpu::SixteenBitPop, this); break;
		case 0x02:f = (i < 0xE0) ? std::bind(&Cpu::Jump, this) : std::bind(&Cpu::EightBitLoad, this); break;
		case 0x04:f = (i < 0xE0) ? std::bind(&Cpu::Call, this) : std::bind(&Cpu::Error, this); break;
		case 0x05:f = std::bind(&Cpu::SixteenBitPush, this); break;
		case 0x07:f = std::bind(&Cpu::Restart, this); break;
		case 0x08:f = (i < 0xE0) ? std::bind(&Cpu::Return, this) : (i < 0xF0) ? std::bind(&Cpu::SixteenBitAdd, this) : std::bind(&Cpu::EightBitLoad, this); break;
		case 0x09:f = (i < 0xE0) ? std::bind(&Cpu::Return, this) : (i < 0xF0) ? std::bind(&Cpu::Jump, this) : std::bind(&Cpu::SixteenBitLoad, this); break;
		case 0x0A:f = (i < 0xE0) ? std::bind(&Cpu::Jump, this) : std::bind(&Cpu::EightBitLoad, this); break;
		case 0x0C:f = (i < 0xE0) ? std::bind(&Cpu::Call, this) : std::bind(&Cpu::Error, this); break;
		case 0x0F:f = std::bind(&Cpu::Restart, this); break;
		default:f = std::bind(&Cpu::Error, this); break;
		}

		functionMap[i] = f;
	}

	//Outside of full columns
	functionMap[0xC3] = std::bind(&Cpu::Jump, this);
	functionMap[0xF3] = std::bind(&Cpu::Misc, this);
	functionMap[0xC6] = std::bind(&Cpu::EightBitAdd, this);
	functionMap[0xD6] = std::bind(&Cpu::EightBitCp, this);
	functionMap[0xE6] = std::bind(&Cpu::EightBitAnd, this);
	functionMap[0xF6] = std::bind(&Cpu::EightBitOr, this);
	functionMap[0xF8] = std::bind(&Cpu::Misc, this);
	functionMap[0xCD] = std::bind(&Cpu::Call, this);
	functionMap[0xCE] = std::bind(&Cpu::EightBitAdc, this);
	functionMap[0xDE] = std::bind(&Cpu::EightBitSbc, this);
	functionMap[0xEE] = std::bind(&Cpu::EightBitXor, this);
	functionMap[0xFB] = std::bind(&Cpu::Misc, this);
	functionMap[0xFE] = std::bind(&Cpu::EightBitCp, this);


	//CB function map
	for (int i = 0x0; i < 0xFF; ++i)
	{
		if (i < 0x30)functionMapCB[i] = std::bind(&Cpu::RotateShift, this);
		else if (i < 0x38)functionMapCB[i] = std::bind(&Cpu::Misc, this);
		else if (i < 0x40)functionMapCB[i] = std::bind(&Cpu::RotateShift, this);
		else functionMapCB[i] = std::bind(&Cpu::BitSet, this);
	}
}

void Cpu::Reset()
{
	CpuHelper::SixteenBitSetRegs(0x01B0, state.A, state.F);
	CpuHelper::SixteenBitSetRegs(0x0013, state.B, state.C);
	CpuHelper::SixteenBitSetRegs(0x00D8, state.D, state.E);
	CpuHelper::SixteenBitSetRegs(0x014D, state.H, state.L);
	state.sp = 0xFFFE;
	state.pc = 0x100;
	state.IME = set;
	state.f_Z = set;
	state.f_N = reset;
	state.f_H = set;
	state.f_C = set;
	state.stop = reset;
	state.halt = reset;
}

void Cpu::CheckInterruptStatus()
{
	//Go through the five different interrupts and see if any is triggered
	if (state.IME == set)
	{
		for (int i = 0; i < 5; i++)
		{
			if (GameBoyHelper::GetByteBit(Memory::GetInstance().GetInterruptEnable(), i) == 1 &&
				GameBoyHelper::GetByteBit(Memory::GetInstance().GetInterruptFlag(), i) == 1)
			{
				Byte restartAddress = 0x0;
				switch (i)
				{
				case 0:restartAddress = 0x40; break;
				case 1:restartAddress = 0x48; break;
				case 2:restartAddress = 0x50; break;
				case 3:restartAddress = 0x58; break;
				case 4:restartAddress = 0x60; break;
				default:std::cout << "Unknown interupt flag?" << std::endl; break;
				}
				RestartOp(restartAddress);
				break;
			}
		}
	}
}
void Cpu::Fetch()
{
	CheckInterruptStatus();
	curOpcode = Memory::GetInstance().ReadByte(state.pc);

	if (curOpcode == breakPointOpCode || state.pc == breakPointPc || state.sp == breakPointSp)
	{
		isDebugging = true;
	}

	//not sure if right placement
	if (IME_disableNextInt)
	{
		state.IME = false;
		IME_disableNextInt = false;
	}
	else if (IME_enableNextInt)
	{
		state.IME = true;
		IME_enableNextInt = false;
	}
}
void Cpu::Execute()
{
	Fetch();
	if (isDebugging)
	{
		PrintState();
	}
	if (isWriting)
	{
		AppendStateToFile("../outputLog.txt");
	}
	if (state.halt == reset && state.stop == reset)
	{

		if (curOpcode == 0xCB)
		{
			state.pc++;
			curOpcode = Memory::GetInstance().ReadByte(state.pc);
			functionMapCB[curOpcode]();
		}
		else
		{
			functionMap[curOpcode]();
		}
	}
	//Update value of F
	GameBoyHelper::SetByteBit(state.F, state.f_Z, 7);
	GameBoyHelper::SetByteBit(state.F, state.f_N, 6);
	GameBoyHelper::SetByteBit(state.F, state.f_H, 5);
	GameBoyHelper::SetByteBit(state.F, state.f_C, 4);
}

//Output
void Cpu::PrintState()
{
	Byte actualOpcode = curOpcode;
	if (actualOpcode == 0xCB)
	{
		actualOpcode = curOpcode = Memory::GetInstance().ReadByte(state.pc + 1);
	}
	std::cout << "PC:" << GameBoyHelper::HexFormatOutX(state.pc) << " - ";
	std::cout << "Opcode: " << GameBoyHelper::HexFormatOutX(curOpcode) << "-" << GameBoyOpcodes[curOpcode] << std::endl;
	std::cout << "-----------" << std::endl;
	std::cout << "A: " << GameBoyHelper::HexFormatOutX(state.A) << " F: " << GameBoyHelper::HexFormatOutX(state.F) << std::endl;
	std::cout << "B: " << GameBoyHelper::HexFormatOutX(state.B) << " C: " << GameBoyHelper::HexFormatOutX(state.C) << std::endl;
	std::cout << "D: " << GameBoyHelper::HexFormatOutX(state.D) << " E: " << GameBoyHelper::HexFormatOutX(state.E) << std::endl;
	std::cout << "H: " << GameBoyHelper::HexFormatOutX(state.H) << " L: " << GameBoyHelper::HexFormatOutX(state.L) << std::endl;
	std::cout << std::endl;
}
void Cpu::AppendStateToFile(std::string path)
{
	Byte actualOpcode = curOpcode;
	if (actualOpcode == 0xCB)
	{
		actualOpcode = curOpcode = Memory::GetInstance().ReadByte(state.pc + 1);
	}
	std::fstream f(path, std::ios::out | std::ios::app);
	if (f.is_open())
	{
		f << "PC:" << GameBoyHelper::HexFormatOutX(state.pc) << " - ";
		f << "Opcode: " << GameBoyHelper::HexFormatOutX(curOpcode) << "-" << GameBoyOpcodes[curOpcode] << std::endl;
		f << "-----------" << std::endl;
		f << "AF:"<< GameBoyHelper::HexFormatOut(state.A) << GameBoyHelper::HexFormatOut(state.F) << std::endl;
		f << "BC:" << GameBoyHelper::HexFormatOut(state.B) <<GameBoyHelper::HexFormatOut(state.C) << std::endl;
		f << "DE:" << GameBoyHelper::HexFormatOut(state.D) << GameBoyHelper::HexFormatOut(state.E) << std::endl;
		f << "HL:" << GameBoyHelper::HexFormatOut(state.H) << GameBoyHelper::HexFormatOut(state.L) << std::endl;
		f << "-----------" << std::endl;
		f << "ZNHC:" << state.f_Z << state.f_N << state.f_H << state.f_C << std::endl;
		f << "\n";
		f.close();
	}
}

//Load
void Cpu::LoadOp(Byte& reg1, Byte reg2)
{
	reg1 = reg2;
	state.pc++;
	UpdateClocks(1, 4);
}
void Cpu::EightBitLoad()
{
	//std::cout << "8bitload" << std::endl;
	switch (curOpcode & 0xF0)
	{
	case 0x00:
		switch (curOpcode & 0x0F)
		{
		case 0x02:Memory::GetInstance().WriteByte(state.A, BC()); state.pc++; UpdateClocks(1, 8); break;
		case 0x06:state.B = GetN(); state.pc += 2; UpdateClocks(2, 8); break;
		case 0x0A: state.A = Memory::GetInstance().ReadByte(BC()); state.pc++; UpdateClocks(1, 8); break;
		case 0x0E:state.C = GetN(); state.pc += 2; UpdateClocks(2, 8); break;
		default: std::cout << "[0x00] unknown 8bit load Opcode 0x" << std::hex << curOpcode << std::endl; break;
		}break;
	case 0x10:
		switch (curOpcode & 0x0F)
		{
		case 0x02:Memory::GetInstance().WriteByte(state.A, DE()); state.pc++; UpdateClocks(1, 8); break;
		case 0x06:state.D = GetN(), state.pc += 2; UpdateClocks(2, 8); break;
		case 0x0A: state.A = Memory::GetInstance().ReadByte(DE()); state.pc++; UpdateClocks(1, 8); break;
		case 0x0E:state.E = GetN(); state.pc += 2; UpdateClocks(2, 8); break;
		default: printf("[0x0010] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x20:
		switch (curOpcode & 0x0F)
		{
		case 0x02:Memory::GetInstance().WriteByte(state.A, HL()); CpuHelper::Increase16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
		case 0x06:state.H = Memory::GetInstance().ReadByte(GetN()); state.pc += 2; UpdateClocks(2, 8); break;
		case 0x0A: state.A = Memory::GetInstance().ReadByte(HL()); CpuHelper::Increase16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
		case 0x0E:state.L = Memory::GetInstance().ReadByte(GetN()); state.pc += 2; UpdateClocks(2, 8); break;
		default: printf("[0x0020] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x30:
		switch (curOpcode & 0x0F)
		{
		case 0x02:Memory::GetInstance().WriteByte(state.A, HL()); CpuHelper::Decrease16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
		case 0x06:Memory::GetInstance().WriteByte(GetN(), HL()); state.pc += 2; UpdateClocks(2, 12); break;
		case 0x0A:state.A = Memory::GetInstance().ReadByte(HL()); CpuHelper::Decrease16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
		case 0x0E:state.A = GetN(); state.pc += 2; UpdateClocks(2, 8); break;
		default: printf("[0x0030] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x40:
		switch (curOpcode & 0x0F)
		{
		case 0x00: LoadOp(state.B, state.B); break;
		case 0x01:  LoadOp(state.B, state.C); break;
		case 0x02:  LoadOp(state.B, state.D);  break;
		case 0x03:  LoadOp(state.B, state.E);   break;
		case 0x04:  LoadOp(state.B, state.H); break;
		case 0x05:  LoadOp(state.B, state.L); break;
		case 0x06:  LoadOp(state.B, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;

		case 0x07:  LoadOp(state.C, state.A); break;
		case 0x08:  LoadOp(state.C, state.B); break;
		case 0x09:  LoadOp(state.C, state.C);  break;
		case 0x0A:  LoadOp(state.C, state.D);  break;
		case 0x0B:  LoadOp(state.C, state.E); break;
		case 0x0C:  LoadOp(state.C, state.H);  break;
		case 0x0D:  LoadOp(state.C, state.L);  break;
		case 0x0E:  LoadOp(state.B, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x0F:  LoadOp(state.B, state.A); break;
		default: printf("[0x0040] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x50:
		switch (curOpcode & 0x0F)
		{
		case 0x00: LoadOp(state.D, state.B); break;
		case 0x01: LoadOp(state.D, state.C); break;
		case 0x02: LoadOp(state.D, state.D); break;
		case 0x03: LoadOp(state.D, state.E); break;
		case 0x04: LoadOp(state.D, state.H); break;
		case 0x05: LoadOp(state.D, state.L); break;
		case 0x06: LoadOp(state.D, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x07: LoadOp(state.D, state.A); break;
		case 0x08: LoadOp(state.E, state.B);  break;
		case 0x09: LoadOp(state.E, state.C); break;
		case 0x0A: LoadOp(state.E, state.D); break;
		case 0x0B: LoadOp(state.E, state.E); break;
		case 0x0C: LoadOp(state.E, state.H); break;
		case 0x0D: LoadOp(state.E, state.L); break;
		case 0x0E: LoadOp(state.E, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x0F: LoadOp(state.E, state.A); break;
		default: printf("[0x0050] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x60:
		switch (curOpcode & 0x0F)
		{
		case 0x00: LoadOp(state.H, state.B); break;
		case 0x01: LoadOp(state.H, state.C); break;
		case 0x02: LoadOp(state.H, state.D); break;
		case 0x03: LoadOp(state.H, state.E);  break;
		case 0x04: LoadOp(state.H, state.H);  break;
		case 0x05: LoadOp(state.H, state.L); break;
		case 0x06: LoadOp(state.H, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x07: LoadOp(state.H, state.A); break;
		case 0x08: LoadOp(state.L, state.B); break;
		case 0x09: LoadOp(state.L, state.C);  break;
		case 0x0A: LoadOp(state.L, state.D);  break;
		case 0x0B: LoadOp(state.L, state.E); break;
		case 0x0C: LoadOp(state.L, state.H); break;
		case 0x0D: LoadOp(state.L, state.L);  break;
		case 0x0E: LoadOp(state.L, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x0F: LoadOp(state.L, state.A); break;
		default: printf("[0x0060] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0x70:
		switch (curOpcode & 0x0F)
		{
		case 0x00: Memory::GetInstance().WriteByte(state.B, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x01: Memory::GetInstance().WriteByte(state.C, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x02: Memory::GetInstance().WriteByte(state.D, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x03: Memory::GetInstance().WriteByte(state.E, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x04: Memory::GetInstance().WriteByte(state.H, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x05: Memory::GetInstance().WriteByte(state.L, HL()); state.pc++;  UpdateClocks(1, 8); break;
		case 0x07: Memory::GetInstance().WriteByte(state.A, HL()); state.pc++; UpdateClocks(1, 8); break;
		case 0x08: LoadOp(state.A, state.B);  break;
		case 0x09: LoadOp(state.A, state.C); break;
		case 0x0A: LoadOp(state.A, state.D);  break;
		case 0x0B: LoadOp(state.A, state.E);  break;
		case 0x0C: LoadOp(state.A, state.H); break;
		case 0x0D: LoadOp(state.A, state.L);  break;
		case 0x0E: LoadOp(state.A, Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
		case 0x0F: LoadOp(state.A, state.A); break;
		default: printf("[0x0070] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0xE0:
		switch (curOpcode & 0x0F)
		{
		case 0x00:Memory::GetInstance().WriteByte(state.A, 0xFF00 + static_cast<Word>(GetN())); state.pc += 2; UpdateClocks(2, 12); break; 
		case 0x02:Memory::GetInstance().WriteByte(state.A, 0xFF00 + static_cast<Word>(state.C)); state.pc++; UpdateClocks(2, 8); break;
		case 0x0A:Memory::GetInstance().WriteByte(state.A, GetNN()); state.pc += 3; UpdateClocks(3, 16); break;
		default: printf("[0x00E0] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	case 0xF0:
		switch (curOpcode & 0x0F)
		{
		case 0x00:state.A = Memory::GetInstance().ReadByte(0xFF00 + static_cast<Word>(GetN())); state.pc += 2; UpdateClocks(2, 12); break;
		case 0x02:state.A = Memory::GetInstance().ReadByte(0xFF00 + static_cast<Word>(state.C)); UpdateClocks(2, 8); break;
		case 0x0A:state.A = Memory::GetInstance().ReadByte(GetNN()); state.pc += 3; UpdateClocks(3, 16); break;
		default: printf("[0xF0] unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
		}break;
	default: printf("unknown 8bit load curOpcode 0x%X\n", curOpcode); break;
	}
}
void Cpu::SixteenBitLoad()
{
	//std::cout << "16bitload" << std::endl;
	switch (curOpcode & 0xFF)
	{
	case 0x01:CpuHelper::SixteenBitSetRegs(GetNN(), state.B, state.C); state.pc += 3; UpdateClocks(3, 12); break;
	case 0x11:CpuHelper::SixteenBitSetRegs(GetNN(), state.D, state.E); state.pc += 3; UpdateClocks(3, 12); break;
	case 0x21:CpuHelper::SixteenBitSetRegs(GetNN(), state.H, state.L); state.pc += 3; UpdateClocks(3, 12); break;
	case 0x31:state.sp = GetNN(); state.pc += 3; UpdateClocks(3, 12); break;
	case 0x08:Memory::GetInstance().WriteWord(state.sp, GetNN()); state.pc += 3; UpdateClocks(3, 20); break;
	case 0xF8:CpuHelper::SixteenBitSetRegs(GetNN() + state.sp, state.H, state.L); state.pc += 2; state.f_Z = reset; state.f_N = reset; UpdateClocks(2, 12); break;
	case 0xF9:state.sp = GameBoyHelper::CombineToWord(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
	default: printf("16bit load curOpcode 0x%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}

//Arithmetic and logics
void Cpu::AddOp(Byte reg)
{
	state.f_N = reset;
	state.f_H = !CpuHelper::isHalfCarry(state.A, reg);
	state.f_C = !CpuHelper::isCarry(state.A, reg);
	state.A += reg;
	state.f_Z = (state.A == 0);
	UpdateClocks(1, 4);
	state.pc++;
}
void Cpu::EightBitAdd()
{
	//std::cout << "8bitAdd" << std::endl;
	switch (curOpcode)
	{
	case 0x80:  break;
	case 0x81: AddOp(state.C); break;
	case 0x82: AddOp(state.D); break;
	case 0x83: AddOp(state.E);  break;
	case 0x84: AddOp(state.H); break;
	case 0x85: AddOp(state.L);  break;
	case 0x86: AddOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0x87: AddOp(state.A); break;
	case 0xC6: AddOp(GetN()); state.pc++; UpdateClocks(2, 8); break;
	default: printf("Unknown add curOpcode 0x%X\n", curOpcode); break;
	}
}

void Cpu::AdcOp(Byte reg2)
{

	state.f_H = !CpuHelper::isHalfCarry(state.A, reg2 + state.f_C);
	Flag oldC = state.f_C;
	state.f_C = !CpuHelper::isCarry(state.A, reg2 + state.f_C);
	state.A += reg2 + oldC;
	UpdateClocks(1, 4);
	state.f_Z = (state.A == 0);
	state.f_N = reset;
	state.pc++;
}
void Cpu::EightBitAdc()
{
	//std::cout << "8bitAdc" << std::endl;
	switch (curOpcode & 0x0F)
	{
	case 0x08:AdcOp(state.B);  break;
	case 0x09:AdcOp(state.C); break;
	case 0x0A:AdcOp(state.D);  break;
	case 0x0B:AdcOp(state.E);  break;
	case 0x0C:AdcOp(state.H);   break;
	case 0x0D:AdcOp(state.L);  break;
	case 0x0E:AdcOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0x0F:AdcOp(state.A); break;
	case 0xCE:AdcOp(GetN()); state.pc++; UpdateClocks(2, 8); break;
	default: printf("Unknown abc operation 0x%X\n"); break;
	}
}

void Cpu::SubOp(Byte reg)
{
	state.f_H = !CpuHelper::isHalfBorrow(state.A, reg);
	state.f_C = !CpuHelper::isBorrow(state.A, reg);
	state.A -= reg;
	state.f_Z = state.A == 0;
	state.f_N = set;
	UpdateClocks(1, 4);
	state.pc++;
}
void Cpu::EightBitSub()
{
	switch (curOpcode)
	{
	case 0x90: SubOp(state.B); break;
	case 0x91: SubOp(state.C); break;
	case 0x92: SubOp(state.D); break;
	case 0x93: SubOp(state.E); break;
	case 0x94: SubOp(state.H); break;
	case 0x95: SubOp(state.L); break;
	case 0x96: SubOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0x97: SubOp(state.A); break;
	case 0xD6: SubOp(GetN()); state.pc++; UpdateClocks(2, 8);  break;
	default: printf("Unknown Sub curOpcode 0x%X\n", curOpcode);
	}
}

void Cpu::SbcOp(Byte reg)
{
	state.f_H = CpuHelper::isHalfBorrow(state.A, reg + state.f_C);
	Flag old = state.f_C;
	state.f_C = CpuHelper::isBorrow(state.A, reg + state.f_C);
	state.A -= (reg + old);
	state.f_Z = state.A == 0;
	state.f_N = set;
	UpdateClocks(1, 4);
	state.pc++;

}
void Cpu::EightBitSbc()
{
	switch (curOpcode & 0x0F)
	{
	case 0x08:SbcOp(state.B); break;
	case 0x09:SbcOp(state.C); break;
	case 0x0A:SbcOp(state.D); break;
	case 0x0B:SbcOp(state.E); break;
	case 0x0C:SbcOp(state.H); break;
	case 0x0D:SbcOp(state.L); break;
	case 0x0E:SbcOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0x0F:SbcOp(state.A); break;
	default: printf("Unknown SBC operation 0x%X\n"); break;
	}
}

void Cpu::IncOp(Byte& reg)
{
	state.f_N = reset;
	state.f_H = CpuHelper::isHalfCarry(reg, 1);
	reg++;
	state.f_Z = (reg == 0);
	UpdateClocks(1, 8);
	state.pc++;
}

void Cpu::EightBitInc()
{
	switch (curOpcode & 0x0F)
	{
	case 0x0C:
		switch (curOpcode & 0xF0)
		{
		case 0x00:IncOp(state.C); break;
		case 0x0010:IncOp(state.E); break;
		case 0x0020:IncOp(state.L); break;
		case 0x0030:IncOp(state.A); break;
		default:printf("Unknown [0x0C] 8bit increase operation 0x%X\n", curOpcode);
		}break;
	case 0x04:
		switch (curOpcode & 0xF0)
		{
		case 0x00:IncOp(state.B); break;
		case 0x0010:IncOp(state.D); break;
		case 0x0020:IncOp(state.H); break;
		case 0x0030:
		{
			Byte hlValue = Memory::GetInstance().ReadByte(HL());
			state.f_H = CpuHelper::isHalfCarry(hlValue, 1);
			Memory::GetInstance().WriteByte(hlValue + 1, HL());
			state.f_Z = 0 == hlValue;
			state.f_N = reset;
			state.pc++;
			UpdateClocks(1, 12);
			break;
		}
		default:printf("Unknown [0x04] 8bit increase operation 0x%X\n", curOpcode); break;
		}break;
	default: printf("Unknown 8bit add command 0x%X\n", curOpcode); break;

	}
}

void Cpu::DecOp(Byte& reg1)
{
	state.f_H = !CpuHelper::isHalfBorrow(reg1, 1);
	reg1--;
	state.f_Z = reg1 == 0;
	state.f_N = set;
	UpdateClocks(1, 8);
	state.pc++;
}

void Cpu::EightBitDec()
{
	switch (curOpcode & 0x0F)
	{
	case 0x0D:
		switch (curOpcode & 0xF0)
		{
		case 0x00:DecOp(state.C); break;
		case 0x10:DecOp(state.E); break;
		case 0x20:DecOp(state.L); break;
		case 0x30:DecOp(state.A); break;
		default:printf("Unknown [0x0C] 8bit increase operation 0x%X\n", curOpcode); break;
		}break;
	case 0x05:
		switch (curOpcode & 0xF0)
		{
		case 0x00:DecOp(state.B); break;
		case 0x10:DecOp(state.D); break;
		case 0x20:DecOp(state.H); break;
		case 0x30:
		{
			Byte hlValue = Memory::GetInstance().ReadByte(HL());
			state.f_H = CpuHelper::isHalfBorrow(hlValue, 1);
			Memory::GetInstance().WriteByte(hlValue - 1, HL());
			state.f_Z = 0 == hlValue;
			state.f_N = reset;
			state.pc++;
			UpdateClocks(1, 12);
			break;
		}
		default:printf("Unknown [0x04] 8bit increase operation 0x%X\n", curOpcode);
		}break;
	default: printf("Unknown 8bit add command 0x%X\n", curOpcode); break;

	}
}

void Cpu::OrOp(Byte reg)
{
	state.A |= reg;
	state.f_Z = 0 == state.A;
	state.f_N = reset;
	state.f_H = reset;
	state.f_C = reset;
	state.pc++;
	UpdateClocks(1, 4);
}
void Cpu::EightBitOr()
{
	switch (curOpcode)
	{
	case 0xB0: OrOp(state.B); break;
	case 0xB1: OrOp(state.C);  break;
	case 0xB2: OrOp(state.D);  break;
	case 0xB3: OrOp(state.E);  break;
	case 0xB4: OrOp(state.H); break;
	case 0xB5: OrOp(state.L); break;
	case 0xB6: OrOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0xB7: OrOp(state.A);  break;
	case 0xF6:OrOp(GetN()); state.pc++; UpdateClocks(2, 8); break;
	default: printf("Unknown opcode 0x%X\n", curOpcode); break;
	}
}

void Cpu::AndOp(Byte reg)
{
	state.A &= reg;
	state.f_Z = 0 == state.A;
	state.f_N = reset;
	state.f_H = set;
	state.f_C = reset;
	state.pc++;
	UpdateClocks(1, 4);
}

void Cpu::EightBitAnd()
{
	switch (curOpcode)
	{
	case 0xA0: AndOp(state.B); break;
	case 0xA1: AndOp(state.C);  break;
	case 0xA2: AndOp(state.D);  break;
	case 0xA3: AndOp(state.E); break;
	case 0xA4: AndOp(state.H); break;
	case 0xA5: AndOp(state.L);  break;
	case 0xA6: AndOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0xA7: AndOp(state.A); break;
	case 0xE6: AndOp(GetN()); state.pc++; UpdateClocks(2, 8);  break;
	default: printf("Unknown and opcode 0x%X\n", curOpcode);
	}
}

void Cpu::XorOp(Byte reg)
{
	state.A ^= reg;
	state.f_Z = 0 == state.A;
	state.f_N = reset;
	state.f_H = reset;
	state.f_C = reset;
	state.pc++;
	UpdateClocks(1, 4);
}

void Cpu::EightBitXor()
{
	switch (curOpcode)
	{

	case 0xA8: XorOp(state.B); break;
	case 0xA9: XorOp(state.C); break;
	case 0xAA: XorOp(state.D); break;
	case 0xAB: XorOp(state.E); break;
	case 0xAC: XorOp(state.H); break;
	case 0xAD: XorOp(state.L); break;
	case 0xAE: XorOp(Memory::GetInstance().ReadByte(HL()));  UpdateClocks(1, 8); break;
	case 0xAF: XorOp(state.A); break;
	case 0xEE: XorOp(GetN()); state.pc++; UpdateClocks(2, 8); break;
	default: printf("Unknown or opcode 0x%X\n", curOpcode); break;
	}
}

void Cpu::CpOp(Byte reg)
{
	state.f_Z = state.A == reg;
	state.f_C = (state.A < reg);
	UpdateClocks(1, 4);
	state.pc++;
}

void Cpu::EightBitCp()
{
	state.f_N = set;
	switch (curOpcode)
	{
	case 0xBF:CpOp(state.A); break;
	case 0xB8:CpOp(state.B); break;
	case 0xB9:CpOp(state.C); break;
	case 0xBA:CpOp(state.D); break;
	case 0xBB:CpOp(state.E); break;
	case 0xBC:CpOp(state.H); break;
	case 0xBD:CpOp(state.L); break;
	case 0xBE:CpOp(Memory::GetInstance().ReadByte(HL())); UpdateClocks(1, 8); break;
	case 0xFE:CpOp(GetN()); UpdateClocks(2, 8); break;
	}
	state.pc++;
}

void Cpu::SixteenBitAdd()
{
	switch (curOpcode)
	{
	case 0x09:state.f_H = CpuHelper::isHalfCarry(HL(), BC()); HL() += BC(); state.f_N = reset; state.pc++; UpdateClocks(1, 8); break;
	case 0x19:state.f_H = CpuHelper::isHalfCarry(HL(), DE()); HL() += DE(); state.f_N = reset; state.pc++; UpdateClocks(1, 8); break;
	case 0x29:state.f_H = CpuHelper::isHalfCarry(HL(), HL()); HL() += HL(); state.f_N = reset; state.pc++; UpdateClocks(1, 8); break;
	case 0x39:state.f_H = CpuHelper::isHalfCarry(HL(), state.sp); HL() += state.sp; state.f_N = reset; state.pc++; UpdateClocks(1, 8); break;
	case 0xE8:state.sp += GetN(); state.f_Z = reset; state.f_N = reset; state.pc += 2; UpdateClocks(2, 16); break;
	}
}
void Cpu::SixteenBitInc()
{
	switch (curOpcode)
	{
	case 0x03:CpuHelper::Increase16bit(state.B, state.C); state.pc++; UpdateClocks(1, 8); break;
	case 0x13:CpuHelper::Increase16bit(state.D, state.E); state.pc++; UpdateClocks(1, 8); break;
	case 0x23:CpuHelper::Increase16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
	case 0x33:state.sp++; state.pc++; UpdateClocks(1, 8); break;
	default: printf("Unknown 16bit inc 0x%X\n", curOpcode); break;
	}
}
void Cpu::SixteenBitDec()
{
	switch (curOpcode)
	{
	case 0x0B:CpuHelper::Decrease16bit(state.B, state.C); state.pc++; UpdateClocks(1, 8); break;
	case 0x1B:CpuHelper::Decrease16bit(state.D, state.E);  state.pc++; UpdateClocks(1, 8); break;
	case 0x2B:CpuHelper::Decrease16bit(state.H, state.L); state.pc++; UpdateClocks(1, 8); break;
	case 0x3B:state.sp--; state.pc++; UpdateClocks(1, 8); break;
	default: printf("Unknown 16bit dec 0x%X\n", curOpcode); break;
	}
}

void Cpu::SixteenBitPop()
{
	switch (curOpcode & 0xF0)
	{
	case 0xF0: PopFromStack(state.A, state.F); state.pc++; UpdateClocks(1, 12); break;
	case 0xC0: PopFromStack(state.B, state.C); state.pc++; UpdateClocks(1, 12); break;
	case 0xD0: PopFromStack(state.D, state.E); state.pc++; UpdateClocks(1, 12); break;
	case 0xE0: PopFromStack(state.H, state.L); state.pc++; UpdateClocks(1, 12); break;
	default: printf("Pop curOpcode 0x%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}
void Cpu::SixteenBitPush()
{
	switch (curOpcode & 0xF0)
	{
	case 0xF0: PushToStack(state.A, state.F); state.pc++; UpdateClocks(1, 16); break;
	case 0xC0: PushToStack(state.B, state.C); state.pc++; UpdateClocks(1, 16); break;
	case 0xD0: PushToStack(state.D, state.E); state.pc++; UpdateClocks(1, 16); break;
	case 0xE0: PushToStack(state.H, state.L); state.pc++; UpdateClocks(1, 16); break;
	default: printf("Push curOpcode 0x%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}
//Misc
void Cpu::Misc()
{
	switch (curOpcode)
	{
	case 0x0: state.pc++; UpdateClocks(1, 4); break;
	case 0x2F: state.A = ~state.A; state.pc++; state.f_N = state.f_H = set; UpdateClocks(1, 4); break;
	case 0x3F: state.f_C ^= 1; state.pc++; UpdateClocks(1, 4); break;
	case 0x37: state.f_C = set; state.pc++;  UpdateClocks(1, 4); break;
	case 0x76: state.halt = set; break;
	case 0x10: state.stop = set; UpdateClocks(2, 4); break;
	case 0xF3: IME_disableNextInt = true; state.pc++; UpdateClocks(1, 4); break;
	case 0xFB: IME_enableNextInt = true; state.pc++; UpdateClocks(1, 4); break;
	case 0x27:
	{
		if ((state.A & 0x0F > 9) || state.f_H == set)
		{
			state.A += 6;
			state.f_H = set;
		}
		if (state.A>0x9F || state.f_C == set)
		{
			state.A += 0x60;
			state.f_C = set;
		}
		state.pc++;
		break;
	}
	default: printf("Misc curOpcode 0x%X does not exist!\n", curOpcode); break;
	}
}

void Cpu::RLCA_Op()
{
	state.f_C = GameBoyHelper::GetByteBit(state.A, 7);
	GameBoyHelper::RotateLeft(state.A, 1);
	state.f_Z = state.A == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(1, 4);
}
//Rotate right through carry
void Cpu::RLA_Op()
{
	Flag newC = GameBoyHelper::GetByteBit(state.A, 7);
	state.A = (state.A << 1) | (state.f_C);
	state.f_C = newC;
	state.f_Z = state.A == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(1, 4);
}
//Rotate right
void Cpu::RRCA_Op()
{
	state.f_C = GameBoyHelper::GetByteBit(state.A, 0);
	GameBoyHelper::RotateRight(state.A, 1);
	state.f_Z = state.A == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(1, 4);
}
void Cpu::RRA_Op()
{
	Byte bigFlag = state.f_C;
	state.f_C = GameBoyHelper::GetByteBit(state.A, 0);
	state.A = (state.A >> 1) | (bigFlag << 7);
	state.f_Z = state.A == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(1, 4);
}
//Rotate left
void Cpu::RLC_Op(Byte& reg)
{
	state.f_C = GameBoyHelper::GetByteBit(reg, 7);
	GameBoyHelper::RotateLeft(reg, 1);
	state.f_Z = reg == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(2, 8);
}
//Rotate left through carry
void Cpu::RL_Op(Byte& reg)
{
	Flag newC = GameBoyHelper::GetByteBit(reg, 7);
	reg = (reg << 1) | (state.f_C);
	state.f_C = newC;
	state.f_Z = reg == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(2, 8);
}
//Rotate right
void Cpu::RRC_Op(Byte& reg)
{
	state.f_C = GameBoyHelper::GetByteBit(reg, 0);
	GameBoyHelper::RotateRight(reg, 1);
	state.f_Z = reg == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(2, 8);
}
//Rotate right through carry
void Cpu::RR_Op(Byte& reg)
{
	Byte bigFlag = state.f_C;
	state.f_C = GameBoyHelper::GetByteBit(reg, 0);
	reg = (reg >> 1) | (bigFlag << 7);
	state.f_Z = state.A == 0;
	state.f_N = reset;
	state.f_H = reset;
	state.pc++;
	UpdateClocks(2, 8);
}
void Cpu::SWAP_Op(Byte& reg)
{
	GameBoyHelper::SwapHexits(reg);
	state.f_C = reset;
	state.f_H = reset;
	state.f_N = reset;
	state.f_Z = reg == 0;
	state.pc++;
	UpdateClocks(2, 8);
}

void Cpu::SLA_Op(Byte& reg)
{
	state.f_C = GameBoyHelper::GetByteBit(reg, 7);
	reg <<= 1;
	state.f_Z = reg == 0;
	state.f_H = reset;
	state.f_N = reset;
	UpdateClocks(2, 8);
}
void Cpu::SRL_Op(Byte& reg)
{
	state.f_C = GameBoyHelper::GetByteBit(reg, 0);
	reg >>= 1;
	state.f_Z = reg == 0;
	state.f_H = reset;
	state.f_N = reset;
	UpdateClocks(2, 8);
}

//Shift without changing MSB
void Cpu::SRA_Op(Byte& reg)
{
	state.f_C = GameBoyHelper::GetByteBit(reg, 0);
	reg = (reg >> 1) | (reg & 80);
	state.f_Z = reg == 0;
	state.f_H = reset;
	state.f_N = reset;
	UpdateClocks(2, 8);
}


void Cpu::TypeAOp(Byte& reg)
{
	switch (curOpcode & 0x0F)
	{
	case 0:RLC_Op(reg); break;
	case 1:RL_Op(reg); break;
	case 2:SLA_Op(reg); break;
	case 3:SWAP_Op(reg);   break;
	}
}
void Cpu::TypeBOp(Byte& reg)
{
	switch (curOpcode & 0x0F)
	{
	case 0:RRC_Op(reg); UpdateClocks(2, 8); break;
	case 1:RR_Op(reg); UpdateClocks(2, 8); break;
	case 2:SRA_Op(reg); UpdateClocks(2, 8); break;
	case 3:SRL_Op(reg); UpdateClocks(2, 8); break;
	}
}

//Rotates, shifts and bit instructions
void Cpu::RotateShift()
{

	switch ((curOpcode & 0xF0) >> 4)
	{
	case 7:TypeAOp(state.A); break;
	case 0:TypeAOp(state.B); break;
	case 1:TypeAOp(state.C); break;
	case 2:TypeAOp(state.D); break;
	case 3:TypeAOp(state.E); break;
	case 4:TypeAOp(state.H); break;
	case 5:TypeAOp(state.L); break;
	case 6:
	{

		Byte mem = Memory::GetInstance().ReadByte(HL());
		TypeAOp(mem);
		Memory::GetInstance().WriteByte(mem, HL());
		UpdateClocks(2, 16);
		break;

	}
	//Columns>7
	case 15:TypeBOp(state.A); break;
	case 8:TypeBOp(state.B); break;
	case 9:TypeBOp(state.C); break;
	case 10:TypeBOp(state.D); break;
	case 11:TypeBOp(state.E); break;
	case 12:TypeBOp(state.H); break;
	case 13:TypeBOp(state.L);  break;
	case 14:
	{
		Byte mem = Memory::GetInstance().ReadByte(HL());
		TypeBOp(mem);
		Memory::GetInstance().WriteByte(mem, HL());
		UpdateClocks(2, 16);
		break;
	}
	default: printf("Rotate shift curOpcode 0xCB%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}

void Cpu::RotatesMisc()
{
	switch (curOpcode & 0xFF)
	{
	case 0x07:RLC_Op(state.A); UpdateClocks(1, 4); break;
	case 0x17:RL_Op(state.A); UpdateClocks(1, 4); break;
	case 0x0F:RRC_Op(state.A); UpdateClocks(1, 4); break;
	case 0x1F:RL_Op(state.A); UpdateClocks(1, 4); break;
	default: printf("Rotate misc curOpcode 0x%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}

void Cpu::BIT_Op(Byte& reg, Byte value)
{
	state.f_Z = GameBoyHelper::GetByteBit(reg, value) == 0;
	state.f_N = reset;
	state.f_H = set;
	state.pc++;
	UpdateClocks(2, 8);
}
void Cpu::SET_Op(Byte& reg, Byte pos)
{
	GameBoyHelper::SetByteBit(reg, set, pos);
	state.pc++;
	UpdateClocks(2, 8);
}
void Cpu::RES_Op(Byte& reg, Byte pos)
{
	GameBoyHelper::SetByteBit(reg, reset, pos);
	state.pc++;
	UpdateClocks(2, 8);
}


void Cpu::BitSet()
{
	//Find the register, instruction and bit used:

	//Register
	Byte usedReg;
	switch (curOpcode & 0x0F)
	{
	case 0xF:
	case 0x7:usedReg = state.A; break;
	case 0x8:
	case 0x0:usedReg = state.B; break;
	case 0x9:
	case 0x1:usedReg = state.C; break;
	case 0xA:
	case 0x2:usedReg = state.D; break;
	case 0xB:
	case 0x3:usedReg = state.E; break;
	case 0xC:
	case 0x4:usedReg = state.H; break;
	case 0xD:
	case 0x5:usedReg = state.L; break;
	}

	//Bit
	Byte bitValue;
	bool leftColumns = curOpcode & 0x0F < 0x08;
	switch (curOpcode & 0xF0)
	{
	case 0x40:
	case 0x80:
	case 0xC0:bitValue = leftColumns ? 0 : 1; break;
	case 0x50:
	case 0x90:
	case 0xD0:bitValue = leftColumns ? 2 : 3; break;
	case 0x60:
	case 0xA0:
	case 0xE0:bitValue = leftColumns ? 4 : 5; break;
	case 0x70:
	case 0xB0:
	case 0xF0:bitValue = leftColumns ? 6 : 7; break;
	}

	//Instruction
	std::function<void(Byte&, Byte const&)> instructionUsed;
	if (curOpcode >= 0x40 && curOpcode < 0x80)
	{
		instructionUsed = std::bind(&Cpu::BIT_Op, this, std::placeholders::_1, std::placeholders::_2);
	}
	else if (curOpcode >= 0x80 && curOpcode < 0xC0)
	{
		instructionUsed = std::bind(&Cpu::RES_Op, this, std::placeholders::_1, std::placeholders::_2);
	}
	else
	{
		instructionUsed = std::bind(&Cpu::SET_Op, this, std::placeholders::_1, std::placeholders::_2);
	}

	if (curOpcode & 0x0F == 0x06 || curOpcode & 0x0F == 0x0E)
	{
		Byte mem = Memory::GetInstance().ReadByte(HL());
		instructionUsed(mem, bitValue);
		Memory::GetInstance().WriteByte(mem, HL());
		UpdateClocks(2, 16);
	}
	else
	{
		instructionUsed(usedReg, bitValue);
	}

}

void Cpu::JumpOp(Flag const& flag, Flag const& value)
{
	if (flag == value)
	{
		UpdateClocks(3, 16);
		state.pc = GetNN();
	}
	else
	{
		UpdateClocks(3, 12);
		state.pc += 3;
	}
}
void Cpu::RelativeJumpOp(Flag const& flag, Flag const& value)
{
	if (flag == value)
	{
		UpdateClocks(2, 12);
		char signedAddress = (char)GetN();
		state.pc += 2;
		state.pc += signedAddress;
	}
	else
	{
		UpdateClocks(2, 8); UpdateClocks(2, 8);
		state.pc += 2;
	}
}

//Jumps, calls, returns and restarts
void Cpu::Jump()
{
	switch (curOpcode)
	{
		//Jumps
	case 0xC3: state.pc = GetNN(); UpdateClocks(3, 16); break;
	case 0xC2:JumpOp(state.f_Z, reset); break;
	case 0xCA:JumpOp(state.f_Z, set); break;
	case 0xD2:JumpOp(state.f_C, reset); break;
	case 0xDA:JumpOp(state.f_C, set); break;
	case 0xE9: state.pc = Memory::GetInstance().ReadByte(HL()); UpdateClocks(1, 4); break;

		//Relative Jumps
	case 0x18:state.pc += GetN(); UpdateClocks(2, 12); break;
	case 0x20:RelativeJumpOp(state.f_Z, reset); break;
	case 0x28:RelativeJumpOp(state.f_Z, set); break;
	case 0x30:RelativeJumpOp(state.f_C, reset); break;
	case 0x38:RelativeJumpOp(state.f_C, set); break;
	default: printf("Jump curOpcode 0x%X does not exist!\n", curOpcode); break;
	}
}

void Cpu::CallOp(Flag const& flag, Flag const& value)
{
	if (flag == value){ UpdateClocks(3, 24); PushPCToStack(); state.pc = GetNN(); }
	else{ UpdateClocks(3, 12); state.pc += 3; }
}

void Cpu::Call()
{
	switch (curOpcode)
	{
	case 0xCD:PushPCToStack(); state.pc = GetNN(); UpdateClocks(3, 24); break;
	case 0xC4:CallOp(state.f_Z, reset); break;
	case 0xCC:CallOp(state.f_Z, set); break;
	case 0xD4:CallOp(state.f_C, reset); break;
	case 0xDC:CallOp(state.f_C, set); break;
	default: printf("Calls curOpcode 0x%X does not exist!\n", curOpcode); if (isDebugging)getchar();
	}
}

void Cpu::RestartOp(Byte address)
{
	PushPCToStack();
	state.pc = address;
	state.IME = reset;
	state.halt = reset;
	UpdateClocks(1, 16);
}
void Cpu::Restart()
{
	switch (curOpcode & 0xFF)
	{
	case 0xC7:RestartOp(0x00); break;
	case 0xCF:RestartOp(0x08); break;
	case 0xD7:RestartOp(0x10); break;
	case 0xDF:RestartOp(0x18); break;
	case 0xE7:RestartOp(0x20); break;
	case 0xEF:RestartOp(0x28); break;
	case 0xF7:RestartOp(0x30); break;
	case 0xFF:RestartOp(0x38); break;
	default: printf("Restarts curOpcode 0x%X does not exist!\n", curOpcode);
	}
}

void Cpu::ReturnOp(Flag const& flag, Flag const& val)
{
	if (flag == val)
	{
		UpdateClocks(1, 20); PopPCFromStack();
	}
	else
	{
		UpdateClocks(1, 8); state.pc++;
	}
}

void Cpu::Return()
{
	switch (curOpcode & 0xFF)
	{
	case 0xC9:PopPCFromStack(); UpdateClocks(1, 16); break;
	case 0xC0:ReturnOp(state.f_Z, reset); break;
	case 0xC8:ReturnOp(state.f_Z, set); break;
	case 0xD0:ReturnOp(state.f_C, reset); break;
	case 0xD8:ReturnOp(state.f_C, set); break;
	case 0xD9:PopPCFromStack(); state.IME=true; UpdateClocks(1, 16); break;
	default: printf("Returns curOpcode 0x%X does not exist!\n", curOpcode); break;
	}
}

//Used to detect errors:
void Cpu::Error()
{
	hasError = true;
	std::cout << "unknown opcode: " << GameBoyHelper::HexFormatOutX(curOpcode) << "-Aborting" << std::endl;
}
