/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "arm.h"

#include "isa-arm.h"
#include "isa-inlines.h"
#include "isa-thumb.h"

//根据处理器工作模式选择寄存器组
static inline enum RegisterBank _ARMSelectBank(enum PrivilegeMode);

//设置处理器工作模式，如有需要改变寄存器配置
void ARMSetPrivilegeMode(struct ARMCore* cpu, enum PrivilegeMode mode) {
	//设置的模式和现在的模式相同，无动作
	if (mode == cpu->privilegeMode) {
		// Not switching modes after all
		return;
	}
	enum RegisterBank newBank = _ARMSelectBank(mode);
	enum RegisterBank oldBank = _ARMSelectBank(cpu->privilegeMode);
	if (newBank != oldBank) {
		// Switch banked registers
		if (mode == MODE_FIQ || cpu->privilegeMode == MODE_FIQ) {
			int oldFIQBank = oldBank == BANK_FIQ;
			int newFIQBank = newBank == BANK_FIQ;
			cpu->bankedRegisters[oldFIQBank][2] = cpu->gprs[8];
			cpu->bankedRegisters[oldFIQBank][3] = cpu->gprs[9];
			cpu->bankedRegisters[oldFIQBank][4] = cpu->gprs[10];
			cpu->bankedRegisters[oldFIQBank][5] = cpu->gprs[11];
			cpu->bankedRegisters[oldFIQBank][6] = cpu->gprs[12];
			cpu->gprs[8] = cpu->bankedRegisters[newFIQBank][2];
			cpu->gprs[9] = cpu->bankedRegisters[newFIQBank][3];
			cpu->gprs[10] = cpu->bankedRegisters[newFIQBank][4];
			cpu->gprs[11] = cpu->bankedRegisters[newFIQBank][5];
			cpu->gprs[12] = cpu->bankedRegisters[newFIQBank][6];
		}
		cpu->bankedRegisters[oldBank][0] = cpu->gprs[ARM_SP];
		cpu->bankedRegisters[oldBank][1] = cpu->gprs[ARM_LR];
		cpu->gprs[ARM_SP] = cpu->bankedRegisters[newBank][0];
		cpu->gprs[ARM_LR] = cpu->bankedRegisters[newBank][1];

		cpu->bankedSPSRs[oldBank] = cpu->spsr.packed;
		cpu->spsr.packed = cpu->bankedSPSRs[newBank];
	}
	//最后一步改变模式设置
	cpu->privilegeMode = mode;
}

//根据处理器工作模式选择寄存器组
static inline enum RegisterBank _ARMSelectBank(enum PrivilegeMode mode) {
	switch (mode) {
		case MODE_USER:
		case MODE_SYSTEM:
			// No banked registers
			return BANK_NONE;
		case MODE_FIQ:
			return BANK_FIQ;
		case MODE_IRQ:
			return BANK_IRQ;
		case MODE_SUPERVISOR:
			return BANK_SUPERVISOR;
		case MODE_ABORT:
			return BANK_ABORT;
		case MODE_UNDEFINED:
			return BANK_UNDEFINED;
		default:
			// This should be unreached
			return BANK_NONE;
	}
}

//进程初始化
void ARMInit(struct ARMCore* cpu) {
	cpu->master->init(cpu, cpu->master);
	int i;
	for (i = 0; i < cpu->numComponents; ++i) {
		cpu->components[i]->init(cpu, cpu->components[i]);
	}
}

//反初始化，析构，销毁对象
void ARMDeinit(struct ARMCore* cpu) {
	if (cpu->master->deinit) {
		cpu->master->deinit(cpu->master);
	}
	int i;
	for (i = 0; i < cpu->numComponents; ++i) {
		if (cpu->components[i]->deinit) {
			cpu->components[i]->deinit(cpu->components[i]);
		}
	}
}

//设置系统进程
void ARMSetComponents(struct ARMCore* cpu, struct ARMComponent* master, int extra, struct ARMComponent** extras) {
	cpu->master = master;
	cpu->numComponents = extra;
	cpu->components = extras;
}

//CPU重置，工作模式进入SYS模式
void ARMReset(struct ARMCore* cpu) {
	int i;
	for (i = 0; i < 16; ++i) {
		cpu->gprs[i] = 0;
	}
	for (i = 0; i < 6; ++i) {
		cpu->bankedRegisters[i][0] = 0;
		cpu->bankedRegisters[i][1] = 0;
		cpu->bankedRegisters[i][2] = 0;
		cpu->bankedRegisters[i][3] = 0;
		cpu->bankedRegisters[i][4] = 0;
		cpu->bankedRegisters[i][5] = 0;
		cpu->bankedRegisters[i][6] = 0;
		cpu->bankedSPSRs[i] = 0;
	}

	cpu->privilegeMode = MODE_SYSTEM;
	cpu->cpsr.packed = MODE_SYSTEM;
	cpu->spsr.packed = 0;

	cpu->shifterOperand = 0;
	cpu->shifterCarryOut = 0;

	cpu->executionMode = MODE_THUMB;
	_ARMSetMode(cpu, MODE_ARM);		//设置CPU工作状态

	int currentCycles = 0;
	ARM_WRITE_PC;		//写ARM程序计数器

	cpu->cycles = 0;
	cpu->nextEvent = 0;
	cpu->halted = 0;

	cpu->irqh.reset(cpu);
}

/*
中断大体步骤：
1.保存当前cpsr寄存器
2.设置CPU工作模式（设置CPU工作模式，并且改变R8-R14和spsr，同时要保存旧的R8-R14和spsr）
3.设置链接寄存器用于恢复现场
4.将程序计数器设置为中断程序入口地址
*/
//拉起IRQ中断
void ARMRaiseIRQ(struct ARMCore* cpu) {
	if (cpu->cpsr.i) {
		return;
	}
	union PSR cpsr = cpu->cpsr;
	int instructionWidth;		//指令宽度
	if (cpu->executionMode == MODE_THUMB) {
		instructionWidth = WORD_SIZE_THUMB;
	} else {
		instructionWidth = WORD_SIZE_ARM;
	}
	ARMSetPrivilegeMode(cpu, MODE_IRQ);
	cpu->cpsr.priv = MODE_IRQ;
	cpu->gprs[ARM_LR] = cpu->gprs[ARM_PC] - instructionWidth + WORD_SIZE_ARM;
	cpu->gprs[ARM_PC] = BASE_IRQ;
	int currentCycles = 0;
	ARM_WRITE_PC;
	cpu->memory.setActiveRegion(cpu, cpu->gprs[ARM_PC]);
	_ARMSetMode(cpu, MODE_ARM);
	cpu->spsr = cpsr;
	cpu->cpsr.i = 1;
}

/*
 * 拉起SWI中断
 *
 * BIOS Halt Functions
 * SWI 04h (GBA) - IntrWait
 * Continues to wait in Halt state until one (or more) of the specified interrupt(s) do occur.
 * The function forcefully sets IME=1. When using multiple interrupts at the same time,
 * this function is having less overhead than repeatedly calling the Halt function.
 * 
 * GBA I/O Map
 * Interrupt, Waitstate, and Power-Down Control
 * 4000200h  2    R/W  IE        Interrupt Enable Register
 * 4000202h  2    R/W  IF        Interrupt Request Flags / IRQ Acknowledge
 * 4000301h  1    W    HALTCNT   Undocumented - Power Down Control
 * 4000208h  2    R/W  IME       Interrupt Master Enable Register
 */
void ARMRaiseSWI(struct ARMCore* cpu) {
	union PSR cpsr = cpu->cpsr;
	int instructionWidth;
	if (cpu->executionMode == MODE_THUMB) {
		instructionWidth = WORD_SIZE_THUMB;
	} else {
		instructionWidth = WORD_SIZE_ARM;
	}
	ARMSetPrivilegeMode(cpu, MODE_SUPERVISOR);
	cpu->cpsr.priv = MODE_SUPERVISOR;
	cpu->gprs[ARM_LR] = cpu->gprs[ARM_PC] - instructionWidth;
	cpu->gprs[ARM_PC] = BASE_SWI;
	int currentCycles = 0;
	ARM_WRITE_PC;
	cpu->memory.setActiveRegion(cpu, cpu->gprs[ARM_PC]);
	_ARMSetMode(cpu, MODE_ARM);
	cpu->spsr = cpsr;
	cpu->cpsr.i = 1;
}

/*
ARM7TDMI芯片 -> ARMv4T指令集
ARM处理器数据处理指令格式：
31-28	27-25	24-21	20  	19-16	15-12	11-0
cond	001		opcode	S		Rn		Rd		Operand2 
cond：执行条件，如EQ，NE等，To execute this only if the zero flag is set:ADDEQ r0,r1,r2(If zero flag set then r0 = r1 + r2)
opcode：操作码，如ADD，SUB等
S：是否影响CPSR寄存器条件标志位的值，主要用于算术运算结果设置（等于0，为负数等），To add two numbers and set the condition flags:ADDS r0,r1,r2(r0 = r1 + r2 and set flags)
Rn：第一个操作数的寄存器编码
Rd：目标寄存器编码
Operand2：第二个操作数，它可以是立即数An immediate value，可以是按值移位的寄存器A register shifted by value，也可以是按寄存器移位的寄存器
*/
//ARM处理器指令执行步骤
static inline void ARMStep(struct ARMCore* cpu) {
	uint32_t opcode = cpu->prefetch;

	LOAD_32(cpu->prefetch, cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion);
	//展开为：cpu->prefetch = ((uint32_t*) cpu->memory.activeRegion)[(cpu->gprs[ARM_PC] & cpu->memory.activeMask) >> 2]

	cpu->gprs[ARM_PC] += WORD_SIZE_ARM;

	unsigned condition = opcode >> 28;
	if (condition != 0xE) {
		bool conditionMet = false;
		switch (condition) {
		case 0x0:
			conditionMet = ARM_COND_EQ;
			break;
		case 0x1:
			conditionMet = ARM_COND_NE;
			break;
		case 0x2:
			conditionMet = ARM_COND_CS;
			break;
		case 0x3:
			conditionMet = ARM_COND_CC;
			break;
		case 0x4:
			conditionMet = ARM_COND_MI;
			break;
		case 0x5:
			conditionMet = ARM_COND_PL;
			break;
		case 0x6:
			conditionMet = ARM_COND_VS;
			break;
		case 0x7:
			conditionMet = ARM_COND_VC;
			break;
		case 0x8:
			conditionMet = ARM_COND_HI;
			break;
		case 0x9:
			conditionMet = ARM_COND_LS;
			break;
		case 0xA:
			conditionMet = ARM_COND_GE;
			break;
		case 0xB:
			conditionMet = ARM_COND_LT;
			break;
		case 0xC:
			conditionMet = ARM_COND_GT;
			break;
		case 0xD:
			conditionMet = ARM_COND_LE;
			break;
		default:
			break;
		}
		if (!conditionMet) {
			cpu->cycles += ARM_PREFETCH_CYCLES;
			return;
		}
	}
	//							 _armTable[ [27-25][24-21][20]0000  |  00000000[7-4] ] = _armTable[ [27-25][24-21][20][7-4] ]
	ARMInstruction instruction = _armTable[((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0x00F)];
	instruction(cpu, opcode);	//除了在内部执行指令外还会计算时钟周期cpu->cycles += ARM_PREFETCH_CYCLES;
}

//Thumb状态的指令执行步骤
static inline void ThumbStep(struct ARMCore* cpu) {
	uint32_t opcode = cpu->prefetch;
	LOAD_16(cpu->prefetch, cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion);
	cpu->gprs[ARM_PC] += WORD_SIZE_THUMB;
	ThumbInstruction instruction = _thumbTable[opcode >> 6];
	instruction(cpu, opcode);
}

//处理器启动运行
void ARMRun(struct ARMCore* cpu) {
	if (cpu->executionMode == MODE_THUMB) {
		ThumbStep(cpu);
	} else {
		ARMStep(cpu);
	}
	if (cpu->cycles >= cpu->nextEvent) {
		cpu->irqh.processEvents(cpu);
	}
}

//指令循环
void ARMRunLoop(struct ARMCore* cpu) {
	if (cpu->executionMode == MODE_THUMB) {
		while (cpu->cycles < cpu->nextEvent) {
			ThumbStep(cpu);
		}
	} else {
		while (cpu->cycles < cpu->nextEvent) {
			ARMStep(cpu);
		}
	}
	cpu->irqh.processEvents(cpu);
}
