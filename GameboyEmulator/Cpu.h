#pragma once
#include "GameboyHelper.h"
#include "Memory.h"

#include <map>
#include <functional>

class Cpu
{

public:
	//Flags:
	typedef bool Flag;
	enum { reset, set };

	inline static Cpu& GetInstance()
	{
		static Cpu instance;
		return instance;
	}
	void Execute();
	void Reset();
	bool const& HasError(){ return hasError; }
	int const& GetClockM()
	{
		return state.clock_m;
	}
	int const& GetClockT()
	{
		return state.clock_t;
	}
	
private:
	Cpu();
	Cpu(Cpu const&) = delete;
	void operator=(Cpu const&) = delete;

	bool hasError = false;

	//Interrupt
	bool IME_enableNextInt = false;
	bool IME_disableNextInt = false;

	typedef struct
	{
		//Stop execution flags
		bool stop;
		bool halt;

		//Clock timers
		Byte clock_m;
		Byte clock_t;

		//Registers
		Byte A, F;
		Byte B, C;
		Byte D, E;
		Byte H, L;

		//Flags
		Flag f_Z;
		Flag f_N;
		Flag f_H;
		Flag f_C;

		//Stack pointer
		Word sp;

		//Program counter
		Word pc;

		//Current opcode:
		Byte opcode;

		//Interupt flags
		Flag IME;
	}CpuState;
	CpuState state;

	//Debugging of execution
	Byte curOpcode;
	int breakPointOpCode = -1;
	int breakPointSp = -1;
	int breakPointPc = -1;
	bool isDebugging = false;
	bool isWriting = true;

	//Function maps
	std::map <int, std::function<void()>> functionMap;
	std::map <int, std::function<void()>> functionMapCB;

	//Execution
	void Fetch();
	void UpdateClocks(int m, int t)
	{ 
		state.clock_m += m; state.clock_t += t; 
		//Add overflow here
	}
	Byte GetN()
	{
		return Memory::GetInstance().ReadByte(state.pc + 1);
	}
	Word GetNN()
	{
		return Memory::GetInstance().ReadWord(state.pc+1);
	}
	void PopPCFromStack() 
	{ 
		state.pc = Memory::GetInstance().ReadWord(state.sp);
		state.sp += 2;
	}
	void PushPCToStack()
	{
		state.sp -= 2;
		Word savedPc = state.pc + 3;
		Memory::GetInstance().WriteWord(savedPc, state.sp);
		savedPc = Memory::GetInstance().ReadWord(state.sp);
	}
	void PopFromStack(Byte& r1, Byte& r2)
	{
		r2 = Memory::GetInstance().ReadByte(state.sp++); 
		r1 = Memory::GetInstance().ReadByte(state.sp++); 
	}
	void PushToStack(Byte  r1, Byte r2)
	{
		state.sp--; 
		Memory::GetInstance().WriteByte(r1,state.sp);
		state.sp--; 
		Memory::GetInstance().WriteByte(r2,state.sp);
	}
	Word& BC(){ return GameBoyHelper::CombineToWord(state.B, state.C); }
	Word& DE(){ return GameBoyHelper::CombineToWord(state.D, state.E); }
	Word& HL(){ return GameBoyHelper::CombineToWord(state.H, state.L); }

	//Output
	void PrintState();
	void AppendStateToFile(std::string);

	void CheckInterruptStatus();

	//Load
	void LoadOp(Byte& reg1, Byte reg2);
	void EightBitLoad();
	void SixteenBitLoad();

	//Arithmetic and logics

	//Addition
	void AddOp(Byte reg2);
	void EightBitAdd();

	//Addiction with carry flag
	void AdcOp(Byte reg2);
	void EightBitAdc();

	//Subtraction
	void SubOp(Byte reg2);
	void EightBitSub();

	//Subtraction with carry flag
	void SbcOp(Byte reg2);
	void EightBitSbc();

	//Increment
	void IncOp(Byte& reg1);
	void EightBitInc();

	//Decrement
	void DecOp(Byte& reg1);
	void EightBitDec();
	
	//Logical operations
	void OrOp(Byte reg2);
	void EightBitOr();
	void AndOp(Byte reg2);
	void EightBitAnd();
	void XorOp(Byte reg2);
	void EightBitXor();
	void CpOp(Byte reg2);
	void EightBitCp();

	//Sixteen bit operations
	void SixteenBitAdd();
	void SixteenBitInc();
	void SixteenBitDec();
	void SixteenBitPop();
	void SixteenBitPush();

	//Misc
	void Misc();

	//Rotates, shifts and bit instructions
	void TypeAOp(Byte& reg);
	void TypeBOp(Byte& reg);
	
	//Rotations
	void RLCA_Op();
	void RLA_Op();
	void RRCA_Op();
	void RRA_Op();

	void RLC_Op(Byte& reg);
	void RL_Op(Byte& reg);
	void RRC_Op(Byte& reg);
	void RR_Op(Byte& reg);
	void SWAP_Op(Byte& reg);
	void RotatesMisc();

	//Shifts
	void SLA_Op(Byte& reg);
	void SRA_Op(Byte& reg);
	void SRL_Op(Byte& reg);
	void RotateShift();

	//Bit
	void BIT_Op(Byte& reg,Byte value);
	void SET_Op(Byte& reg, Byte value);
	void RES_Op(Byte& reg, Byte value);
	void BitSet();
	
	
	//Jumps, calls, returns and restarts
	void JumpOp(Flag const& flag, Flag const& value);
	void RelativeJumpOp(Flag const& flag, Flag const& value);
	void Jump();
	void CallOp(Flag const& flag, Flag const& value);
	void Call();
	void RestartOp(Byte address);
	void Restart();
	void ReturnOp(Flag const& flag, Flag const& val);
	void Return();

	//Used to detect errors:
	void Error();

};

