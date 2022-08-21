/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ISA_INLINES_H
#define ISA_INLINES_H

#include "macros.h"

#include "arm.h"

/*
每条ARM指令都包含4位条件码，位于指令最高4位[31:28]，通过条件码和cpsr寄存器的条件标志位的对比决定是否执行当前指令

CMP r0 #0    这条指令（比较指令就会更新cpsr中相应条件标志位）执行完后cpsr的Z标志位可能被置位1（r0=0）
ADDEQ r1, r1, #1    条件执行指令，若前一条指令使得cpsr的Z标志位被置位1则该指令执行
ADDNE r2, r2, #1    条件执行指令，若前一条指令使得cpsr的Z标志位仍未0则该指令执行

cpsr寄存器条件标志位的意义如下：
N：负数，改变标志位的最后的ALU操作产生负数结果（32位结果的最高位是1）
Z：零，改变标志位的最后的ALU操作产生0结果（32位结果的每一位都是0）
C：进位，改变标志位的最后的ALU操作产生到符号位的进位
V：溢出，改变标志位的最后的ALU操作产生到符号位的溢出
*/

//Z置位执行，相等
#define ARM_COND_EQ (cpu->cpsr.z)
//Z清零执行，不相等
#define ARM_COND_NE (!cpu->cpsr.z)
//C置位执行，无符号数大于或等于
#define ARM_COND_CS (cpu->cpsr.c)
//C清零执行，无符号数小于
#define ARM_COND_CC (!cpu->cpsr.c)
//N置位执行，负数
#define ARM_COND_MI (cpu->cpsr.n)
//N清零执行，正数或零
#define ARM_COND_PL (!cpu->cpsr.n)
//V置位执行，溢出
#define ARM_COND_VS (cpu->cpsr.v)
//V清零执行，未溢出
#define ARM_COND_VC (!cpu->cpsr.v)
//C置位Z清零，无符号数大于
#define ARM_COND_HI (cpu->cpsr.c && !cpu->cpsr.z)
//C清零Z置位，无符号数小于或等于
#define ARM_COND_LS (!cpu->cpsr.c || cpu->cpsr.z)
//N等于V，带符号数大于或等于
#define ARM_COND_GE (!cpu->cpsr.n == !cpu->cpsr.v)
//N不等于V，带符号数小于
#define ARM_COND_LT (!cpu->cpsr.n != !cpu->cpsr.v)
//Z清零且N等于V，带符号数大于
#define ARM_COND_GT (!cpu->cpsr.z && !cpu->cpsr.n == !cpu->cpsr.v)
//Z置位或N不等于V，带符号小于或等于
#define ARM_COND_LE (cpu->cpsr.z || !cpu->cpsr.n != !cpu->cpsr.v)
//忽略，无条件执行
#define ARM_COND_AL 1

//取数据符号位，即最高位
#define ARM_SIGN(I) ((I) >> 31)
//循环右移操作
#define ARM_ROR(I, ROTATE) ((((uint32_t) (I)) >> ROTATE) | ((uint32_t) (I) << ((-ROTATE) & 31)))

//加法运算带来进位
#define ARM_CARRY_FROM(M, N, D) (((uint32_t) (M) >> 31) + ((uint32_t) (N) >> 31) > ((uint32_t) (D) >> 31))
//减法运算带来借位
#define ARM_BORROW_FROM(M, N, D) (((uint32_t) (M)) >= ((uint32_t) (N)))
//加法溢出
#define ARM_V_ADDITION(M, N, D) (!(ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))) && (ARM_SIGN((N) ^ (D))))
//减法溢出
#define ARM_V_SUBTRACTION(M, N, D) ((ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))))

//计算乘法指令的时钟周期数
#define ARM_WAIT_MUL(R) \
	if ((R & 0xFFFFFF00) == 0xFFFFFF00 || !(R & 0xFFFFFF00)) { \
		currentCycles += 1; \
	} else if ((R & 0xFFFF0000) == 0xFFFF0000 || !(R & 0xFFFF0000)) { \
		currentCycles += 2; \
	} else if ((R & 0xFF000000) == 0xFF000000 || !(R & 0xFF000000)) { \
		currentCycles += 3; \
	} else { \
		currentCycles += 4; \
	}

#define ARM_STUB cpu->irqh.hitStub(cpu, opcode)
#define ARM_ILL cpu->irqh.hitIllegal(cpu, opcode)

//写ARM状态所用的程序计数器
#define ARM_WRITE_PC \
	cpu->gprs[ARM_PC] = (cpu->gprs[ARM_PC] & -WORD_SIZE_ARM); \
	cpu->memory.setActiveRegion(cpu, cpu->gprs[ARM_PC]); \
	LOAD_32(cpu->prefetch, cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion); \
	cpu->gprs[ARM_PC] += WORD_SIZE_ARM; \
	currentCycles += 2 + cpu->memory.activeUncachedCycles32 + cpu->memory.activeSeqCycles32;

//写THUMB状态所用的程序计数器
#define THUMB_WRITE_PC \
	cpu->gprs[ARM_PC] = (cpu->gprs[ARM_PC] & -WORD_SIZE_THUMB); \
	cpu->memory.setActiveRegion(cpu, cpu->gprs[ARM_PC]); \
	LOAD_16(cpu->prefetch, cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion); \
	cpu->gprs[ARM_PC] += WORD_SIZE_THUMB; \
	currentCycles += 2 + cpu->memory.activeUncachedCycles16 + cpu->memory.activeSeqCycles16;

//当前模式下是否有SPSR寄存器（SYSTEM和USER模式下没有SPSR寄存器）
static inline int _ARMModeHasSPSR(enum PrivilegeMode mode) {
	return mode != MODE_SYSTEM && mode != MODE_USER;
}

//设置CPU工作状态
static inline void _ARMSetMode(struct ARMCore* cpu, enum ExecutionMode executionMode) {
	if (executionMode == cpu->executionMode) {
		return;
	}

	cpu->executionMode = executionMode;
	switch (executionMode) {
	case MODE_ARM:
		cpu->cpsr.t = 0;
		break;
	case MODE_THUMB:
		cpu->cpsr.t = 1;
	}
	cpu->nextEvent = 0;
}

//读当前程序状态寄存器
static inline void _ARMReadCPSR(struct ARMCore* cpu) {
	_ARMSetMode(cpu, cpu->cpsr.t);
	ARMSetPrivilegeMode(cpu, cpu->cpsr.priv);
	cpu->irqh.readCPSR(cpu);
}

#endif
