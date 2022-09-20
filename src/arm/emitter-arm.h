/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef EMITTER_ARM_H
#define EMITTER_ARM_H

#include "emitter-inlines.h"

#define DECLARE_INSTRUCTION_ARM(EMITTER, NAME) \
	EMITTER ## NAME

#define DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ALU) \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## I)), \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## I))

/*
宏定义：DECLARE_ARM_ALU_BLOCK(EMITTER, ALU, EX1, EX2, EX3, EX4)
扩展之后：
		EMITTER ## ALU ## _LSL		==>		EMITTERALU_LSL
		EMITTER ## ALU ## _LSLR		==>		EMITTERALU_LSLR
		EMITTER ## ALU ## _LSR		==>		EMITTERALU_LSR
		EMITTER ## ALU ## _LSRR		==>		EMITTERALU_LSRR
		EMITTER ## ALU ## _ASR		==>		EMITTERALU_ASR
		EMITTER ## ALU ## _ASRR		==>		EMITTERALU_ASRR
		EMITTER ## ALU ## _ROR		==>		EMITTERALU_ROR
		EMITTER ## ALU ## _RORR		==>		EMITTERALU_RORR
		EMITTER ## ALU ## _LSL		==>		EMITTERALU_LSL
		EMITTER ## EX1				==>		EMITTEREX1
		EMITTER ## ALU ## _LSR		==>		EMITTERALU_LSR
		EMITTER ## EX2				==>		EMITTEREX2
		EMITTER ## ALU ## _ASR		==>		EMITTERALU_ASR
		EMITTER ## EX3				==>		EMITTEREX3
		EMITTER ## ALU ## _ROR		==>		EMITTERALU_ROR
		EMITTER ## EX4				==>		EMITTEREX4
*/
#define DECLARE_ARM_ALU_BLOCK(EMITTER, ALU, EX1, EX2, EX3, EX4) \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSLR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSRR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _ASR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _ASRR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _ROR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _RORR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, EX1), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _LSR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, EX2), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _ASR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, EX3), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ALU ## _ROR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, EX4)

#define DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, NAME, P, U, W) \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## I ## P ## U ## W)), \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## I ## P ## U ## W))

#define DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, NAME, P, U, W) \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _LSL_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _LSR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _ASR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _ROR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _LSL_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _LSR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _ASR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## _ROR_ ## P ## U ## W), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL)

#define DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, NAME, MODE, W) \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## MODE ## W)), \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME ## MODE ## W))

#define DECLARE_ARM_BRANCH_BLOCK(EMITTER, NAME) \
	DO_256(DECLARE_INSTRUCTION_ARM(EMITTER, NAME))

// TODO: Support coprocessors，支持协处理器
#define DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, NAME, P, U, N, W) \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME)), \
	DO_8(DECLARE_INSTRUCTION_ARM(EMITTER, NAME))

#define DECLARE_ARM_COPROCESSOR_BLOCK(EMITTER, NAME1, NAME2) \
	DO_8(DO_8(DO_INTERLACE(DECLARE_INSTRUCTION_ARM(EMITTER, NAME1), DECLARE_INSTRUCTION_ARM(EMITTER, NAME2))))

#define DECLARE_ARM_SWI_BLOCK(EMITTER) \
	DO_256(DECLARE_INSTRUCTION_ARM(EMITTER, SWI))

/*
原宏定义：DECLARE_ARM_EMITTER_BLOCK(EMITTER)
扩展之后：
		......
		DECLARE_ARM_ALU_BLOCK(EMITTER, ADD, UMULL, STRHU, ILL, ILL), \
		......

		EMITTER ## ADD ## _LSL		==>		EMITTERADD_LSL
		EMITTER ## ADD ## _LSLR		==>		EMITTERADD_LSLR
		EMITTER ## ADD ## _LSR		==>		EMITTERADD_LSR
		EMITTER ## ADD ## _LSRR		==>		EMITTERADD_LSRR
		EMITTER ## ADD ## _ASR		==>		EMITTERADD_ASR
		EMITTER ## ADD ## _ASRR		==>		EMITTERADD_ASRR
		EMITTER ## ADD ## _ROR		==>		EMITTERADD_ROR
		EMITTER ## ADD ## _RORR		==>		EMITTERADD_RORR
		EMITTER ## ADD ## _LSL		==>		EMITTERADD_LSL
		EMITTER ## UMULL			==>		EMITTERUMULL
		EMITTER ## ADD ## _LSR		==>		EMITTERADD_LSR
		EMITTER ## STRHU			==>		EMITTERSTRHU
		EMITTER ## ADD ## _ASR		==>		EMITTERADD_ASR
		EMITTER ## ILL				==>		EMITTERILL
		EMITTER ## ADD ## _ROR		==>		EMITTERADD_ROR
		EMITTER ## ILL				==>		EMITTERILL
*/

// 16 * 16 + 1 * 16
#define DECLARE_ARM_EMITTER_BLOCK(EMITTER) \
	DECLARE_ARM_ALU_BLOCK(EMITTER, AND, MUL, STRH, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ANDS, MULS, LDRH, LDRSB, LDRSH), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, EOR, MLA, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, EORS, MLAS, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, SUB, ILL, STRHI, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, SUBS, ILL, LDRHI, LDRSBI, LDRSHI), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, RSB, ILL, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, RSBS, ILL, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ADD, UMULL, STRHU, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ADDS, UMULLS, LDRHU, LDRSBU, LDRSHU), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ADC, UMLAL, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ADCS, UMLALS, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, SBC, SMULL, STRHIU, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, SBCS, SMULLS, LDRHIU, LDRSBIU, LDRSHIU), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, RSC, SMLAL, ILL, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, RSCS, SMLALS, ILL, ILL, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, MRS), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, SWP), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, STRHP), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \							// 17 * 16
	DECLARE_ARM_ALU_BLOCK(EMITTER, TST, ILL, LDRHP, LDRSBP, LDRSHP), \	// 18 * 16
	DECLARE_INSTRUCTION_ARM(EMITTER, MSR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, BX), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, BKPT), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, STRHPW), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \							//19 * 16
	DECLARE_ARM_ALU_BLOCK(EMITTER, TEQ, ILL, LDRHPW, LDRSBPW, LDRSHPW), \	// 20 * 16
	DECLARE_INSTRUCTION_ARM(EMITTER, MRSR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, SWPB), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, STRHIP), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \								// 21 * 16
	DECLARE_ARM_ALU_BLOCK(EMITTER, CMP, ILL, LDRHIP, LDRSBIP, LDRSHIP), \	// 22 * 16
	DECLARE_INSTRUCTION_ARM(EMITTER, MSRR), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, STRHIPW), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \
	DECLARE_INSTRUCTION_ARM(EMITTER, ILL), \								//23 * 16
	DECLARE_ARM_ALU_BLOCK(EMITTER, CMN, ILL, LDRHIPW, LDRSBIPW, LDRSHIPW), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ORR, SMLAL, STRHPU, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, ORRS, SMLALS, LDRHPU, LDRSBPU, LDRSHPU), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, MOV, SMLAL, STRHPUW, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, MOVS, SMLALS, LDRHPUW, LDRSBPUW, LDRSHPUW), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, BIC, SMLAL, STRHIPU, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, BICS, SMLALS, LDRHIPU, LDRSBIPU, LDRSHIPU), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, MVN, SMLAL, STRHIPUW, ILL, ILL), \
	DECLARE_ARM_ALU_BLOCK(EMITTER, MVNS, SMLALS, LDRHIPUW, LDRSBIPUW, LDRSHIPUW), \	// 32 * 16
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, AND), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ANDS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, EOR), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, EORS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, SUB), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, SUBS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, RSB), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, RSBS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ADD), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ADDS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ADC), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ADCS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, SBC), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, SBCS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, RSC), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, RSCS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, TST), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, TST), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MSR), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, TEQ), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, CMP), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, CMP), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MSRR), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, CMN), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ORR), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, ORRS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MOV), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MOVS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, BIC), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, BICS), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MVN), \
	DECLARE_ARM_ALU_IMMEDIATE_BLOCK(EMITTER, MVNS), \							// 64 * 16
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRT, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRT, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRBT, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRBT, , , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRT, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRT, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRBT, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRBT, , U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, P, , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, P, , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, P, , W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, P, , W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, P, , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, P, , ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, P, , W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, P, , W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, P, U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, P, U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STR, P, U, W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDR, P, U, W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, P, U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, P, U, ), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, STRB, P, U, W), \
	DECLARE_ARM_LOAD_STORE_IMMEDIATE_BLOCK(EMITTER, LDRB, P, U, W), \		// 96 * 16
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRT, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRT, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRBT, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRBT, , , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRT, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRT, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRBT, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRBT, , U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, P, , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, P, , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, P, , W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, P, , W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, P, , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, P, , ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, P, , W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, P, , W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, P, U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, P, U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STR, P, U, W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDR, P, U, W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, P, U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, P, U, ), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, STRB, P, U, W), \
	DECLARE_ARM_LOAD_STORE_BLOCK(EMITTER, LDRB, P, U, W), \				// 128 * 16
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, DA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, DA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, DA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, DA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, DA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, DA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, DA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, DA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, IA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, IA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, IA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, IA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, IA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, IA, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, IA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, IA, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, DB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, DB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, DB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, DB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, DB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, DB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, DB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, DB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, IB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, IB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STM, IB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDM, IB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, IB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, IB, ), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, STMS, IB, W), \
	DECLARE_ARM_LOAD_STORE_MULTIPLE_BLOCK(EMITTER, LDMS, IB, W), \		// 160 * 16
	DECLARE_ARM_BRANCH_BLOCK(EMITTER, B), \
	DECLARE_ARM_BRANCH_BLOCK(EMITTER, BL), \							// 672 * 16
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , , , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , , , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , , , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , , , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , , N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , , N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , , N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , , N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , U, , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , U, , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , U, , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , U, , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, , U, N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, , U, N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, , , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, , , ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, , , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, , , W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, U, N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, U, N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, , N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, , N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, , N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, , N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, U, N, ), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, STC, P, U, N, W), \
	DECLARE_ARM_LOAD_STORE_COPROCESSOR_BLOCK(EMITTER, LDC, P, U, N, W), \	// 705 * 16
	DECLARE_ARM_COPROCESSOR_BLOCK(EMITTER, CDP, MCR), \
	DECLARE_ARM_COPROCESSOR_BLOCK(EMITTER, CDP, MRC), \						// 705 + 128 * 16
	DECLARE_ARM_SWI_BLOCK(EMITTER)											// 705 + 128 + 256 * 16 == 1098 * 16

#endif
