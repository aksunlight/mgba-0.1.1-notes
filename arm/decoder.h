/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//解码
#ifndef ARM_DECODER_H
#define ARM_DECODER_H

#include "arm.h"
//operand 操作数	opcode 操作码	operation 操作
// Bit 0: a register is involved with this operand这个操作数涉及一个寄存器，意思就是操作数从寄存器中取
// Bit 1: an immediate is invovled with this operand这个操作数涉及一个立即数，意思就是操作数就是立即数
// Bit 2: a memory access is invovled with this operand这个操作数涉及一块内存访问，意思就是操作数从内存中取
// Bit 3: the destination of this operand is affected by this opcode操作码不同操作数也从不同地方取
// Bit 4: this operand is shifted by a register这个操作数被一个寄存器改变
// Bit 5: this operand is shifted by an immediate这个操作数被被一个立即数改变
#define ARM_OPERAND_NONE                0x00000000	//指令没有操作数
#define ARM_OPERAND_REGISTER_1          0x00000001  //指令有一个寄存器操作数
#define ARM_OPERAND_IMMEDIATE_1         0x00000002	//指令有一个立即数操作数
#define ARM_OPERAND_MEMORY_1            0x00000004	//指令有一个内存操作数
#define ARM_OPERAND_AFFECTED_1          0x00000008	//指令有一个受操作码影响的操作数
#define ARM_OPERAND_SHIFT_REGISTER_1    0x00000010	//指令的一个被寄存器改变的操作数
#define ARM_OPERAND_SHIFT_IMMEDIATE_1   0x00000020	//指令有一个被立即数改变的操作数
#define ARM_OPERAND_1                   0x000000FF	//指令有一个操作数

#define ARM_OPERAND_REGISTER_2          0x00000100
#define ARM_OPERAND_IMMEDIATE_2         0x00000200
#define ARM_OPERAND_MEMORY_2            0x00000400
#define ARM_OPERAND_AFFECTED_2          0x00000800
#define ARM_OPERAND_SHIFT_REGISTER_2    0x00001000
#define ARM_OPERAND_SHIFT_IMMEDIATE_2   0x00002000
#define ARM_OPERAND_2                   0x0000FF00

#define ARM_OPERAND_REGISTER_3          0x00010000
#define ARM_OPERAND_IMMEDIATE_3         0x00020000
#define ARM_OPERAND_MEMORY_3            0x00040000
#define ARM_OPERAND_AFFECTED_3          0x00080000
#define ARM_OPERAND_SHIFT_REGISTER_3    0x00100000
#define ARM_OPERAND_SHIFT_IMMEDIATE_3   0x00200000
#define ARM_OPERAND_3                   0x00FF0000

#define ARM_OPERAND_REGISTER_4          0x01000000
#define ARM_OPERAND_IMMEDIATE_4         0x02000000
#define ARM_OPERAND_MEMORY_4            0x04000000
#define ARM_OPERAND_AFFECTED_4          0x08000000
#define ARM_OPERAND_SHIFT_REGISTER_4    0x10000000
#define ARM_OPERAND_SHIFT_IMMEDIATE_4   0x20000000
#define ARM_OPERAND_4                   0xFF000000


#define ARM_MEMORY_REGISTER_BASE     0x0001		//内存基地址，存于寄存器
#define ARM_MEMORY_IMMEDIATE_OFFSET  0x0002		//内存偏移地址，存于立即数
#define ARM_MEMORY_REGISTER_OFFSET   0x0004		//内存偏移地址，存于寄存器
#define ARM_MEMORY_SHIFTED_OFFSET    0x0008		//内存偏移量
#define ARM_MEMORY_PRE_INCREMENT     0x0010		//预增量
#define ARM_MEMORY_POST_INCREMENT    0x0020		//后增量
#define ARM_MEMORY_OFFSET_SUBTRACT   0x0040		//减去偏移量
#define ARM_MEMORY_WRITEBACK         0x0080		//写回
#define ARM_MEMORY_DECREMENT_AFTER   0x0000		//之后递减
#define ARM_MEMORY_INCREMENT_AFTER   0x0100		//之后递增
#define ARM_MEMORY_DECREMENT_BEFORE  0x0200		//之前递减
#define ARM_MEMORY_INCREMENT_BEFORE  0x0300		//之前递增
#define ARM_MEMORY_SPSR_SWAP         0x0400		

#define MEMORY_FORMAT_TO_DIRECTION(F) (((F) >> 8) & 0x3)

enum ARMCondition {		//ARM指令执行条件
	ARM_CONDITION_EQ = 0x0,
	ARM_CONDITION_NE = 0x1,
	ARM_CONDITION_CS = 0x2,
	ARM_CONDITION_CC = 0x3,
	ARM_CONDITION_MI = 0x4,
	ARM_CONDITION_PL = 0x5,
	ARM_CONDITION_VS = 0x6,
	ARM_CONDITION_VC = 0x7,
	ARM_CONDITION_HI = 0x8,
	ARM_CONDITION_LS = 0x9,
	ARM_CONDITION_GE = 0xA,
	ARM_CONDITION_LT = 0xB,
	ARM_CONDITION_GT = 0xC,
	ARM_CONDITION_LE = 0xD,
	ARM_CONDITION_AL = 0xE,
	ARM_CONDITION_NV = 0xF
};

enum ARMShifterOperation {		//ARM移位操作，LSL逻辑左移 LSR逻辑右移 ASL算术左移 ASR算术右移 ROR循环右移 RRX带扩展的循环右移
	ARM_SHIFT_NONE = 0,
	ARM_SHIFT_LSL,
	ARM_SHIFT_LSR,
	ARM_SHIFT_ASR,
	ARM_SHIFT_ROR,
	ARM_SHIFT_RRX
};

union ARMOperand {		//指令第二操作数Op2？第一操作数+目的操作数+第二操作数？第一操作数+第二操作数？
	struct {
		uint8_t reg;	//1st operand register
		uint8_t shifterOp;	//位移模式，从ARMShifterOperation中取值
		union {
			uint8_t shifterReg;	//operand 2 is a register
			uint8_t shifterImm;	//operand 2 is an immediate value
		};
	};
	int32_t immediate;
};

enum ARMMemoryAccessType {		//内存访问类型
	ARM_ACCESS_WORD = 4,		//字类型，4字节
	ARM_ACCESS_HALFWORD = 2,	//半字类型，2字节
	ARM_ACCESS_SIGNED_HALFWORD = 10,	//有符号半字
	ARM_ACCESS_BYTE = 1,	//字节
	ARM_ACCESS_SIGNED_BYTE = 9,		//有符号字节
	ARM_ACCESS_TRANSLATED_WORD = 20,
	ARM_ACCESS_TRANSLATED_BYTE = 17
};

enum ARMBranchType {		//ARM分支类型，跳转的类型
	ARM_BRANCH_NONE = 0,	//不跳转
	ARM_BRANCH = 1,			//普通跳转
	ARM_BRANCH_INDIRECT = 2,	//间接跳转？
	ARM_BRANCH_LINKED = 4		//连接跳转？
};

struct ARMMemoryAccess {	//ARM内存访问
	uint8_t baseReg;	//基址
	uint8_t width;		//大小
	uint16_t format;	//形式
	union ARMOperand offset;	//偏移量
};

enum ARMMnemonic {		//ARM操作码（助记符）
	ARM_MN_ILL = 0,
	ARM_MN_ADC,
	ARM_MN_ADD,
	ARM_MN_AND,
	ARM_MN_ASR,
	ARM_MN_B,
	ARM_MN_BIC,
	ARM_MN_BKPT,
	ARM_MN_BL,
	ARM_MN_BLH,
	ARM_MN_BX,
	ARM_MN_CMN,
	ARM_MN_CMP,
	ARM_MN_EOR,
	ARM_MN_LDM,
	ARM_MN_LDR,
	ARM_MN_LSL,
	ARM_MN_LSR,
	ARM_MN_MLA,
	ARM_MN_MOV,
	ARM_MN_MRS,
	ARM_MN_MSR,
	ARM_MN_MUL,
	ARM_MN_MVN,
	ARM_MN_NEG,
	ARM_MN_ORR,
	ARM_MN_ROR,
	ARM_MN_RSB,
	ARM_MN_RSC,
	ARM_MN_SBC,
	ARM_MN_SMLAL,
	ARM_MN_SMULL,
	ARM_MN_STM,
	ARM_MN_STR,
	ARM_MN_SUB,
	ARM_MN_SWI,
	ARM_MN_SWP,
	ARM_MN_TEQ,
	ARM_MN_TST,
	ARM_MN_UMLAL,
	ARM_MN_UMULL,

	ARM_MN_MAX
};

enum {
	ARM_CPSR = 16,
	ARM_SPSR = 17
};

struct ARMInstructionInfo {		//指令信息
	uint32_t opcode;
	union ARMOperand op1;
	union ARMOperand op2;
	union ARMOperand op3;
	union ARMOperand op4;
	struct ARMMemoryAccess memory;
	int operandFormat;
	unsigned execMode : 1;
	bool traps : 1;
	bool affectsCPSR : 1;
	unsigned branchType : 3;
	unsigned condition : 4;
	unsigned mnemonic : 6;
	unsigned iCycles : 3;
	unsigned cCycles : 4;
	unsigned sInstructionCycles : 4;
	unsigned nInstructionCycles : 4;
	unsigned sDataCycles : 10;
	unsigned nDataCycles : 10;
};

void ARMDecodeARM(uint32_t opcode, struct ARMInstructionInfo* info);	//解码32位指令
void ARMDecodeThumb(uint16_t opcode, struct ARMInstructionInfo* info);	//解码16位指令
int ARMDisassemble(struct ARMInstructionInfo* info, uint32_t pc, char* buffer, int blen);	//解码指令

#endif
