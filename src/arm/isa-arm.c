/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "isa-arm.h"

#include "arm.h"
#include "emitter-arm.h"
#include "isa-inlines.h"

#define PSR_USER_MASK   0xF0000000 //条件标志位
#define PSR_PRIV_MASK   0x000000CF //中断使能位和工作模式
#define PSR_STATE_MASK  0x00000020 //工作状态标志位

/*
Addressing mode 1：Shifter operands for data processing instructions(数据处理指令中的**移位操作数**的计算方法)
首先，数据处理指令或者说使用移位操作数的指令有：ADD ADC SUB SBC RSB RSC CMP CMN TST TEQ AND EOR ORR BIC MOV MVN

数据处理指令助记符：<Opcode>{<Cond>}{S} <Rd>, <Rn>, <Operand2>，注意：<Operand2>即<shifter_operand>！
<Operand2>具体形式如下：
			#<immediate>
			<Rm>
			<Rm>, LSL #<shift_imm>
			<Rm>, LSL <Rs>
			<Rm>, LSR #<shift_imm>
			<Rm>, LSR <Rs>
			<Rm>, ASR #<shift_imm>
			<Rm>, ASR <Rs>
			<Rm>, ROR #<shift_imm>
			<Rm>, ROR <Rs>
			<Rm>, RRX

数据处理指令格式：Cond(31-28) 0 0 0/1 Opcode(24-21) S(20) Rn(19-16) Rd(15-12) **Operand2(11-0)**
对于不同Operand2，指令编码如下：
32-bit immediate：cond 0 0 1 opcode S Rn Rd **rotate_imm(4) immed_8**
Immediate shifts：cond 0 0 0 opcode S Rn Rd **shift_imm(5) shift(2) 0 Rm**
Register shifts： cond 0 0 0 opcode S Rn Rd **Rs 0 shift(2) 1 Rm**

对于S标志位：如果指令中的S位置位，那么会更新CPSR中相应的条件标志位，如果目的寄存器又是R15(PC)，还会将当前模式的SPSR恢复到CPSR中(该指令的运算结果并不影响CPSR的标志位)
1.数据处理指令中如果第二源操作数是一个寄存器(0-3位)则可以在其值送入ALU之前先对该寄存器的值进行移位操作(也可不移)，移位数可以是立即数(7-11位)也可以由寄存器(8-11位)给出
  如果由寄存器给出则指令第4位为1，第7位为0，如果由寄存器给出则指令第4位为0，移位方式(5-6位)实际有LSL逻辑左移 LSR逻辑右移 ASR算数右移 ROR循环右移，ASL等同LSL，RRX并入ROR
2.数据处理指令中如果第二源操作数是一个32位立即(0-7位)数则将第0-7位作为立即数种子immed，第8-11位作为移位因子rot，送往ALU的数为immed循环右移2*rot位(xxxx0，2的倍数，偶数)的结果

For non-addition/subtractions that incorporate a shift operation, C is set to the last bit 
shifted out of the value by the shifter. As well as producing the shifter operand, 
the shifter produces a carry-out which some instructions write into the Carry Flag.
注意：移位操作会改变C标志位，大部分情况C标志位被设为移位器移位出的最后一位的值！
具体来说，移位操作使得移位器产生进位carry-out，这个进位一般是移位器移出的最后一位的值，这个值被一些指令用来设置C标志位！
*/

//逻辑左移，当Operand2是一个无移位的寄存器时，实际就是该寄存器逻辑左移立即数0
static inline void _shiftLSL(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int immediate = (opcode & 0x00000F80) >> 7;
	if (!immediate) {    //LSL 0位
		cpu->shifterOperand = cpu->gprs[rm];
		//当Operan2是一个无移位的寄存器或者一个寄存器逻辑左移立即数0，移位器产生的进位就是C标志位
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else {    		//LSL 1-31位
		cpu->shifterOperand = cpu->gprs[rm] << immediate;
		cpu->shifterCarryOut = (cpu->gprs[rm] >> (32 - immediate)) & 1;
	}
}

//使用寄存器保存移位数的逻辑左移
static inline void _shiftLSLR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int rs = (opcode >> 8) & 0x0000000F;
	++cpu->cycles;
	int32_t shiftVal = cpu->gprs[rm];
	if (rm == ARM_PC) {
		shiftVal += 4;
	}
	int shift = cpu->gprs[rs] & 0xFF;
	if (!shift) {				//LSL 0位
		cpu->shifterOperand = shiftVal;
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else if (shift < 32) {	//LSL 1-31位
		cpu->shifterOperand = shiftVal << shift;
		cpu->shifterCarryOut = (shiftVal >> (32 - shift)) & 1;
	} else if (shift == 32) {	//LSL 32位
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = shiftVal & 1;
	} else {					//LSL 32-0xFF位
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = 0;
	}
}

//逻辑右移
static inline void _shiftLSR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int immediate = (opcode & 0x00000F80) >> 7;
	if (immediate) {	//LSR 1-31位
		cpu->shifterOperand = ((uint32_t) cpu->gprs[rm]) >> immediate;
		cpu->shifterCarryOut = (cpu->gprs[rm] >> (immediate - 1)) & 1;
	} else {  			//注意是LSR 32位，A shift by 32 is encoded by shift_imm == 0
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = ARM_SIGN(cpu->gprs[rm]);
	}
}

//使用寄存器保存移位数的逻辑右移
static inline void _shiftLSRR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int rs = (opcode >> 8) & 0x0000000F;
	++cpu->cycles;
	uint32_t shiftVal = cpu->gprs[rm];
	if (rm == ARM_PC) {
		shiftVal += 4;
	}
	int shift = cpu->gprs[rs] & 0xFF;
	if (!shift) {
		cpu->shifterOperand = shiftVal;
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else if (shift < 32) {
		cpu->shifterOperand = shiftVal >> shift;
		cpu->shifterCarryOut = (shiftVal >> (shift - 1)) & 1;
	} else if (shift == 32) {
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = shiftVal >> 31;
	} else {
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = 0;
	}
}

//算数右移
static inline void _shiftASR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int immediate = (opcode & 0x00000F80) >> 7;
	if (immediate) {	//ASR 1-31位
		cpu->shifterOperand = cpu->gprs[rm] >> immediate;
		cpu->shifterCarryOut = (cpu->gprs[rm] >> (immediate - 1)) & 1;
	} else {			//注意是ASR 32位，A shift by 32 is encoded by shift_imm == 0
		cpu->shifterCarryOut = ARM_SIGN(cpu->gprs[rm]);
		//按手册的话cpu->shifterOperand = 0xFFFFFFFF或0，这里是1或0?
		cpu->shifterOperand = cpu->shifterCarryOut;
	}
}

//使用寄存器保存移位数的算术右移
static inline void _shiftASRR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int rs = (opcode >> 8) & 0x0000000F;
	++cpu->cycles;
	int shiftVal =  cpu->gprs[rm];
	if (rm == ARM_PC) {
		shiftVal += 4;
	}
	int shift = cpu->gprs[rs] & 0xFF;
	if (!shift) {
		cpu->shifterOperand = shiftVal;
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else if (shift < 32) {
		cpu->shifterOperand = shiftVal >> shift;
		cpu->shifterCarryOut = (shiftVal >> (shift - 1)) & 1;
	} else if (cpu->gprs[rm] >> 31) {   //对于负数，算数右移32位或以上
		cpu->shifterOperand = 0xFFFFFFFF;
		cpu->shifterCarryOut = 1;
	} else {    						//对于正数，算数右移32位或以上
		cpu->shifterOperand = 0;
		cpu->shifterCarryOut = 0;
	}
}

//循环右移
static inline void _shiftROR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int immediate = (opcode & 0x00000F80) >> 7;
	if (immediate) {    //ROR 1-31位
		cpu->shifterOperand = ARM_ROR(cpu->gprs[rm], immediate);
		//The carry-out from the shifter is the last bit rotated off the right end.
		cpu->shifterCarryOut = (cpu->gprs[rm] >> (immediate - 1)) & 1;
	} else {    		//当ROR 0位时转为RRX指令
		//RRX, This data-processing operand can be used to perform a 33-bit rotate right using the Carry Flag as 
		//the 33rd bit. This instruction operand is the value of register Rm shifted right by one bit, with the Carry
        //Flag replacing the vacated bit position. The carry-out from the shifter is the bit shifted off the right end.
		cpu->shifterOperand = (cpu->cpsr.c << 31) | (((uint32_t) cpu->gprs[rm]) >> 1);
		cpu->shifterCarryOut = cpu->gprs[rm] & 0x00000001;
	}
}

//使用寄存器保存移位数的循环右移
static inline void _shiftRORR(struct ARMCore* cpu, uint32_t opcode) {
	int rm = opcode & 0x0000000F;
	int rs = (opcode >> 8) & 0x0000000F;
	++cpu->cycles;
	int shiftVal = cpu->gprs[rm];
	if (rm == ARM_PC) {
		shiftVal += 4;
	}
	int shift = cpu->gprs[rs] & 0xFF;
	int rotate = shift & 0x1F;
	if (!shift) {			//ROR 0位
		cpu->shifterOperand = shiftVal;
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else if (rotate) {    //ROR 1-31位
		cpu->shifterOperand = ARM_ROR(shiftVal, rotate);
		cpu->shifterCarryOut = (shiftVal >> (rotate - 1)) & 1;
	} else {   				//ROR 32位或32的倍数
		cpu->shifterOperand = shiftVal;
		cpu->shifterCarryOut = ARM_SIGN(shiftVal);
	}
}

//立即数循环右移
//rule: The <shifter_operand> value is formed by rotating (to the right) an 8-bit immediate value to
//any even bit position in a 32-bit word. **If the rotate immediate is zero, the carry-out from the
//shifter is the value of the C flag, otherwise, it is set to bit[31] of the value of <shifter_operand>**
static inline void _immediate(struct ARMCore* cpu, uint32_t opcode) {
	int rotate = (opcode & 0x00000F00) >> 7;
	int immediate = opcode & 0x000000FF;
	if (!rotate) {	//ROR 0位
		cpu->shifterOperand = immediate;
		cpu->shifterCarryOut = cpu->cpsr.c;
	} else {		//ROR 1-31位
		cpu->shifterOperand = ARM_ROR(immediate, rotate);
		cpu->shifterCarryOut = ARM_SIGN(cpu->shifterOperand);
	}
}

//Instruction definitions，指令定义
//Beware pre-processor antics，担心指令预取异常

//数据扩展成64位
#define NO_EXTEND64(V) (uint64_t)(uint32_t) (V)

//设置了S标志将更新CPSR中相应的条件标志位，如果目的寄存器又是R15(PC)
//还会将当前模式的SPSR恢复到CPSR中(该指令的运算结果并不影响CPSR的标志位)

//设置了S标志位的数据处理指令
#define ARM_ADDITION_S(M, N, D) \
	if (rd == ARM_PC && _ARMModeHasSPSR(cpu->cpsr.priv)) { \
		cpu->cpsr = cpu->spsr; \
		_ARMReadCPSR(cpu); \
	} else { \
		cpu->cpsr.n = ARM_SIGN(D); \
		cpu->cpsr.z = !(D); \
		cpu->cpsr.c = ARM_CARRY_FROM(M, N, D); \
		cpu->cpsr.v = ARM_V_ADDITION(M, N, D); \
	}

//设置了S标志的减法指令
#define ARM_SUBTRACTION_S(M, N, D) \
	if (rd == ARM_PC && _ARMModeHasSPSR(cpu->cpsr.priv)) { \
		cpu->cpsr = cpu->spsr; \
		_ARMReadCPSR(cpu); \
	} else { \
		cpu->cpsr.n = ARM_SIGN(D); \
		cpu->cpsr.z = !(D); \
		cpu->cpsr.c = ARM_BORROW_FROM(M, N, D); \
		cpu->cpsr.v = ARM_V_SUBTRACTION(M, N, D); \
	}

//设置了S标志为的??指令	
#define ARM_NEUTRAL_S(M, N, D) \
	if (rd == ARM_PC && _ARMModeHasSPSR(cpu->cpsr.priv)) { \
		cpu->cpsr = cpu->spsr; \
		_ARMReadCPSR(cpu); \
	} else { \
		cpu->cpsr.n = ARM_SIGN(D); \
		cpu->cpsr.z = !(D); \
		cpu->cpsr.c = cpu->shifterCarryOut; \
	}

//??
#define ARM_NEUTRAL_HI_S(DLO, DHI) \
	cpu->cpsr.n = ARM_SIGN(DHI); \
	cpu->cpsr.z = !((DHI) | (DLO));

/*
v4架构所有数据加载/存储指令：LDR LDRT LDRB LDRBT LDRH LDRSB LDRSH STR STRT STRB STRBT STRH
其中涉及半字操作和有符号字节操作的指令(LDRH LDRSH LDRSB STRH)使用3号寻址模式(Addresing Mode 3)
涉及字和无符号字节操作的指令(LDR LDRT LDRB LDRBT STR STRT STRB STRBT)使用2号寻址模式(Addresing Mode 2)
Addressing Mode 2：Load and Store Word or Unsigned Byte(字和无符号字节加载/存储指令中**内存地址**的计算方法)
Addressing Mode 3：Load and store halfword or load signed byte(半字和有符号字节加载/存储指令中**内存地址**的计算方法)

字和无符号字节加载/存储指令助记符：LDR|STR{<Cond>}{B}{T} <Rd>, <addressing_mode>
<addressing_mode>具体形式如下：
			[<Rn>, #+/-<offset_12>]{!}
			[<Rn>, +/-<Rm>]{!}
			[<Rn>, +/-<Rm>, <shift> #<shift_imm>]{!}
			[<Rn>], #+/-<offset_12>
			[<Rn>], +/-<Rm>
			[<Rn>], +/-<Rm>, <shift> #<shift_imm>
注意：对于前三种有!则更新基址寄存器Rn的值(带写回)，无!则不更新Rn(不带写回)，后三种会更新Rn的值
	 前三种先计算出地址再用它来跟新Rn(pre-index)，后三种先使用Rn作为地址再给Rn计算新的地址(post-index)
	 LDRBT, LDRT, STRBT, and STRT only support post-indexed addressing modes.(带T的仅支持后三种)

半字和有符号字节加载/存储指令助记符：LDR|STR{<Cond>}H|SH|SB|D <Rd>, <addressing_mode>
<addressing_mode>具体形式如下：
			[<Rn>, #+/-<offset_8>]{!}
			[<Rn>, +/-<Rm>]{!}
			[<Rn>], #+/-<offset_8>
			[<Rn>], +/-<Rm>
注意事项同上，只是少了几种寻址方式

字和无符号字节加载/存储指令格式：Cond(31-28) 0 1 0/1 P U B W L Rn(19-16) Rd(15-12) **addressing_mode(11-0)**
对于不同addressing_mode，指令编码如下：
Immediate offset：            Cond 0 1 0 1 U B 0 L Rn Rd **offset_12**    [<Rn>, #+/-<offset_12>]
Immediate pre-indexed：       Cond 0 1 0 1 U B 1 L Rn Rd **offset_12**    [<Rn>, #+/-<offset_12>]!
Register offset：             Cond 0 1 1 1 U B 0 L Rn Rd **0 0 0 0 0 0 0 0 Rm**    [<Rn>, +/-<Rm>] 
Register pre-indexed：        Cond 0 1 1 1 U B 1 L Rn Rd **0 0 0 0 0 0 0 0 Rm**    [<Rn>, +/-<Rm>]! 
Scaled register offset：      Cond 0 1 1 1 U B 0 L Rn Rd **shift_imm(5) shift(2) 0 Rm**    [<Rn>, +/-<Rm>, LSL #<shift_imm>]
Scaled register pre-indexed： Cond 0 1 1 1 U B 1 L Rn Rd **shift_imm(5) shift(2) 0 Rm**    [<Rn>, +/-<Rm>, LSL #<shift_imm>]!
Immediate post-indexed：      Cond 0 1 0 0 U B 0 L Rn Rd **offset_12**             [<Rn>], #+/-<offset_12>
Register post-indexed：       Cond 0 1 1 0 U B 0 L Rn Rd **0 0 0 0 0 0 0 0 Rm**    [<Rn>], +/-<Rm>
Scaled register post-indexed：Cond 0 1 1 0 U B 0 L Rn Rd **shift_imm(5) shift(2) 0 Rm**    [<Rn>], +/-<Rm>, LSL #<shift_imm>

半字和有符号字节加载/存储指令格式：Cond(31-28) 0 0 0 P U B W L Rn(19-16) Rd(15-12) **addressing_mode(11-0)**
对于不同addressing_mode，指令编码如下：
Immediate offset：            Cond 0 0 0 P U 1 W L Rn Rd **immedH(4) 1 S H 1 ImmedL(4)**       [<Rn>, #+/-<offset_8>]
Immediate pre-indexed：       Cond 0 0 0 1 U 1 1 L Rn Rd **immedH(4) 1 S H 1 ImmedL(4)**       [<Rn>, #+/-<offset_8>]!
Register offset：             Cond 0 0 0 1 U 0 0 L Rn Rd **SBZ(4) 1 S H 1 Rm(4)**              [<Rn>, +/-<Rm>]
Register pre-indexed：        Cond 0 0 0 1 U 0 1 L Rn Rd **SBZ(4) 1 S H 1 Rm(4)**              [<Rn>, +/-<Rm>]!
Immediate post-indexed:       Cond 0 0 0 0 U 1 0 L Rn Rd **immedH(4) 1 S H 1 ImmedL(4)**       [<Rn>], #+/-<offset_8>
Register post-indexed：       Cond 0 0 0 0 U 0 0 L Rn Rd **SBZ(4) 1 S H 1 Rm(4)**              [<Rn>], +/-<Rm>


Addressing Mode 4：Load and Store Multiple(加载/存储多个指令中**基址增长**的计算方法)
加载/存储多个指令助记符：LDM|STM{<cond>}<addressing_mode> <Rn>{!}, <registers>{^}
<addressing_mode>具体形式如下：
			IA (Increment After)
			IB (Increment Before)
			DA (Decrement After)
			DB (Decrement Before)
注意：有!表示更新基址寄存器Rn的值(将最后的地址写入)，无!则不更新Rn(不带写回)

加载/存储多个指令格式：Cond 1 0 0 P U S W L Rn register-list(16)
对于不同addressing_mode，指令编码如下：
IA(Increment After)：         Cond 1 0 0 0 1 S W L Rn register-list
IB(Increment Before)：        Cond 1 0 0 1 1 S W L Rn register-list
DA(Decrement After)：         Cond 1 0 0 0 0 S W L Rn register-list
DB(Decrement before)：        Cond 1 0 0 1 0 S W L Rn register-list
*/
#define ADDR_MODE_2_I_TEST (opcode & 0x00000F80)
#define ADDR_MODE_2_I ((opcode & 0x00000F80) >> 7)
#define ADDR_MODE_2_ADDRESS (address)
#define ADDR_MODE_2_RN (cpu->gprs[rn])	//第一源操作数
#define ADDR_MODE_2_RM (cpu->gprs[rm])  //第二源操作数
#define ADDR_MODE_2_IMMEDIATE (opcode & 0x00000FFF)
#define ADDR_MODE_2_INDEX(U_OP, M) (cpu->gprs[rn] U_OP M)    //变址方式
#define ADDR_MODE_2_WRITEBACK(ADDR) (cpu->gprs[rn] = ADDR)   //带写回
#define ADDR_MODE_2_LSL (cpu->gprs[rm] << ADDR_MODE_2_I)
#define ADDR_MODE_2_LSR (ADDR_MODE_2_I_TEST ? ((uint32_t) cpu->gprs[rm]) >> ADDR_MODE_2_I : 0)
#define ADDR_MODE_2_ASR (ADDR_MODE_2_I_TEST ? ((int32_t) cpu->gprs[rm]) >> ADDR_MODE_2_I : ((int32_t) cpu->gprs[rm]) >> 31)
#define ADDR_MODE_2_ROR (ADDR_MODE_2_I_TEST ? ARM_ROR(cpu->gprs[rm], ADDR_MODE_2_I) : (cpu->cpsr.c << 31) | (((uint32_t) cpu->gprs[rm]) >> 1))

#define ADDR_MODE_3_ADDRESS ADDR_MODE_2_ADDRESS
#define ADDR_MODE_3_RN ADDR_MODE_2_RN
#define ADDR_MODE_3_RM ADDR_MODE_2_RM
#define ADDR_MODE_3_IMMEDIATE (((opcode & 0x00000F00) >> 4) | (opcode & 0x0000000F))
#define ADDR_MODE_3_INDEX(U_OP, M) ADDR_MODE_2_INDEX(U_OP, M)
#define ADDR_MODE_3_WRITEBACK(ADDR) ADDR_MODE_2_WRITEBACK(ADDR)

//LDM指令带写回(更新基址寄存器Rn)
#define ADDR_MODE_4_WRITEBACK_LDM \
		if (!((1 << rn) & rs)) { \
			cpu->gprs[rn] = address; \
		}

//STM指令带写回(更新基址寄存器Rn)
#define ADDR_MODE_4_WRITEBACK_STM cpu->gprs[rn] = address;

#define ARM_LOAD_POST_BODY \
	++currentCycles; \
	if (rd == ARM_PC) { \
		ARM_WRITE_PC; \
	}

#define ARM_STORE_POST_BODY \
	currentCycles += cpu->memory.activeNonseqCycles32 - cpu->memory.activeSeqCycles32;

//定义ARM指令的公共部分
#define DEFINE_INSTRUCTION_ARM(NAME, BODY) \
	static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) { \
		int currentCycles = ARM_PREFETCH_CYCLES; \
		BODY; \
		cpu->cycles += currentCycles; \
	}

/*
宏定义：DEFINE_ALU_INSTRUCTION_EX_ARM(NAME, S_BODY, SHIFTER, BODY)
替换文本：
static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rn = (opcode >> 16) & 0xF;
		UNUSED(rn);
		SHIFTER(cpu, opcode);
		BODY;
		S_BODY;
		if (rd == ARM_PC) {
			if (cpu->executionMode == MODE_ARM) {
				ARM_WRITE_PC;
			} else {
				THUMB_WRITE_PC;
			}
		}
		cpu->cycles += currentCycles;
	}
*/
//算数逻辑指令需要在ARM指令宏上扩展,
//数据处理指令格式：Cond(31-28) 0 0 0/1 Opcode(24-21) S(20) Rn(19-16) Rd(15-12) **Operand2(11-0)**
#define DEFINE_ALU_INSTRUCTION_EX_ARM(NAME, S_BODY, SHIFTER, BODY) \
	DEFINE_INSTRUCTION_ARM(NAME, \
		int rd = (opcode >> 12) & 0xF; \
		int rn = (opcode >> 16) & 0xF; \
		UNUSED(rn); \
		SHIFTER(cpu, opcode); \
		BODY; \
		S_BODY; \
		if (rd == ARM_PC) { \
			if (cpu->executionMode == MODE_ARM) { \
				ARM_WRITE_PC; \
			} else { \
				THUMB_WRITE_PC; \
			} \
		})

/*
宏定义：DEFINE_ALU_INSTRUCTION_ARM(NAME, S_BODY, BODY)
替换文本：
**DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSL, , _shiftLSL, BODY)**
static void _ARMInstruction ## NAME ## _LSL (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rn = (opcode >> 16) & 0xF;
		UNUSED(rn);
		_shiftLSL(cpu, opcode);
		BODY;
		;
		if (rd == ARM_PC) {
			if (cpu->executionMode == MODE_ARM) {
				ARM_WRITE_PC;
			} else {
				THUMB_WRITE_PC;
			}
		}
		cpu->cycles += currentCycles;
	}
**DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_LSL, S_BODY, _shiftLSL, BODY)**
static void _ARMInstruction ## NAME ## S_LSL (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rn = (opcode >> 16) & 0xF;
		UNUSED(rn);
		_shiftLSL(cpu, opcode);
		BODY;
		S_BODY;
		if (rd == ARM_PC) {
			if (cpu->executionMode == MODE_ARM) {
				ARM_WRITE_PC;
			} else {
				THUMB_WRITE_PC;
			}
		}
		cpu->cycles += currentCycles;
	}
	......
*/
//定义算数逻辑运算(算术、逻辑、比较)指令的宏
#define DEFINE_ALU_INSTRUCTION_ARM(NAME, S_BODY, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSL, , _shiftLSL, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_LSL, S_BODY, _shiftLSL, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSLR, , _shiftLSLR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_LSLR, S_BODY, _shiftLSLR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSR, , _shiftLSR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_LSR, S_BODY, _shiftLSR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSRR, , _shiftLSRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_LSRR, S_BODY, _shiftLSRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ASR, , _shiftASR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_ASR, S_BODY, _shiftASR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ASRR, , _shiftASRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_ASRR, S_BODY, _shiftASRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ROR, , _shiftROR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_ROR, S_BODY, _shiftROR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _RORR, , _shiftRORR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## S_RORR, S_BODY, _shiftRORR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## I, , _immediate, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## SI, S_BODY, _immediate, BODY)

#define DEFINE_ALU_INSTRUCTION_S_ONLY_ARM(NAME, S_BODY, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSL, S_BODY, _shiftLSL, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSLR, S_BODY, _shiftLSLR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSR, S_BODY, _shiftLSR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _LSRR, S_BODY, _shiftLSRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ASR, S_BODY, _shiftASR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ASRR, S_BODY, _shiftASRR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _ROR, S_BODY, _shiftROR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## _RORR, S_BODY, _shiftRORR, BODY) \
	DEFINE_ALU_INSTRUCTION_EX_ARM(NAME ## I, S_BODY, _immediate, BODY)

/*
宏定义：DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME, BODY, S_BODY)
替换文本：
static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rdHi = (opcode >> 16) & 0xF;
		int rs = (opcode >> 8) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rdHi);
		ARM_WAIT_MUL(cpu->gprs[rs]);
		BODY;
		S_BODY;
		if (rd == ARM_PC) {
			ARM_WRITE_PC;
		}
		cpu->cycles += currentCycles;
	}
*/
//乘法指令需要在ARM指令宏上扩展
#define DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME, BODY, S_BODY) \
	DEFINE_INSTRUCTION_ARM(NAME, \
		int rd = (opcode >> 12) & 0xF; \
		int rdHi = (opcode >> 16) & 0xF; \
		int rs = (opcode >> 8) & 0xF; \
		int rm = opcode & 0xF; \
		UNUSED(rdHi); \
		ARM_WAIT_MUL(cpu->gprs[rs]); \
		BODY; \
		S_BODY; \
		if (rd == ARM_PC) { \
			ARM_WRITE_PC; \
		})
/*
宏定义：DEFINE_MULTIPLY_INSTRUCTION_ARM(NAME, BODY, S_BODY)
替换文本：
**DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME, BODY, )**
static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rdHi = (opcode >> 16) & 0xF;
		int rs = (opcode >> 8) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rdHi);
		ARM_WAIT_MUL(cpu->gprs[rs]);
		BODY;
		;
		if (rd == ARM_PC) {
			ARM_WRITE_PC;
		}
		cpu->cycles += currentCycles;
	}
**DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME ## S, BODY, S_BODY)**
static void _ARMInstruction ## NAME ## S (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rdHi = (opcode >> 16) & 0xF;
		int rs = (opcode >> 8) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rdHi);
		ARM_WAIT_MUL(cpu->gprs[rs]);
		BODY;
		S_BODY;
		if (rd == ARM_PC) {
			ARM_WRITE_PC;
		}
		cpu->cycles += currentCycles;
	}
*/
//定义乘法运算指令的宏
#define DEFINE_MULTIPLY_INSTRUCTION_ARM(NAME, BODY, S_BODY) \
	DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME, BODY, ) \
	DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME ## S, BODY, S_BODY)

/*
宏定义：DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME, ADDRESS, WRITEBACK, BODY)
替换文本：
static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		uint32_t address;
		int rn = (opcode >> 16) & 0xF;
		int rd = (opcode >> 12) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rm);
		address = ADDRESS;
		WRITEBACK;
		BODY;
		cpu->cycles += currentCycles;
	}
*/
//数据加载/存储指令需要在ARM指令宏上扩展
#define DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME, ADDRESS, WRITEBACK, BODY) \
	DEFINE_INSTRUCTION_ARM(NAME, \
		uint32_t address; \
		int rn = (opcode >> 16) & 0xF; \
		int rd = (opcode >> 12) & 0xF; \
		int rm = opcode & 0xF; \
		UNUSED(rm); \
		address = ADDRESS; \
		WRITEBACK; \
		BODY;)

//定义数据加载/存储指令的宏
#define DEFINE_LOAD_STORE_INSTRUCTION_SHIFTER_ARM(NAME, SHIFTER, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(-, SHIFTER)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## U, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(+, SHIFTER)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## P, ADDR_MODE_2_INDEX(-, SHIFTER), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PW, ADDR_MODE_2_INDEX(-, SHIFTER), ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_ADDRESS), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PU, ADDR_MODE_2_INDEX(+, SHIFTER), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PUW, ADDR_MODE_2_INDEX(+, SHIFTER), ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_ADDRESS), BODY)

#define DEFINE_LOAD_STORE_INSTRUCTION_ARM(NAME, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_SHIFTER_ARM(NAME ## _LSL_, ADDR_MODE_2_LSL, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_SHIFTER_ARM(NAME ## _LSR_, ADDR_MODE_2_LSR, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_SHIFTER_ARM(NAME ## _ASR_, ADDR_MODE_2_ASR, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_SHIFTER_ARM(NAME ## _ROR_, ADDR_MODE_2_ROR, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## I, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(-, ADDR_MODE_2_IMMEDIATE)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IU, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(+, ADDR_MODE_2_IMMEDIATE)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IP, ADDR_MODE_2_INDEX(-, ADDR_MODE_2_IMMEDIATE), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPW, ADDR_MODE_2_INDEX(-, ADDR_MODE_2_IMMEDIATE), ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_ADDRESS), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPU, ADDR_MODE_2_INDEX(+, ADDR_MODE_2_IMMEDIATE), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPUW, ADDR_MODE_2_INDEX(+, ADDR_MODE_2_IMMEDIATE), ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_ADDRESS), BODY) \

#define DEFINE_LOAD_STORE_MODE_3_INSTRUCTION_ARM(NAME, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME, ADDR_MODE_3_RN, ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_INDEX(-, ADDR_MODE_3_RM)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## U, ADDR_MODE_3_RN, ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_INDEX(+, ADDR_MODE_3_RM)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## P, ADDR_MODE_3_INDEX(-, ADDR_MODE_3_RM), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PW, ADDR_MODE_3_INDEX(-, ADDR_MODE_3_RM), ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_ADDRESS), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PU, ADDR_MODE_3_INDEX(+, ADDR_MODE_3_RM), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## PUW, ADDR_MODE_3_INDEX(+, ADDR_MODE_3_RM), ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_ADDRESS), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## I, ADDR_MODE_3_RN, ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_INDEX(-, ADDR_MODE_3_IMMEDIATE)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IU, ADDR_MODE_3_RN, ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_INDEX(+, ADDR_MODE_3_IMMEDIATE)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IP, ADDR_MODE_3_INDEX(-, ADDR_MODE_3_IMMEDIATE), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPW, ADDR_MODE_3_INDEX(-, ADDR_MODE_3_IMMEDIATE), ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_ADDRESS), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPU, ADDR_MODE_3_INDEX(+, ADDR_MODE_3_IMMEDIATE), , BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IPUW, ADDR_MODE_3_INDEX(+, ADDR_MODE_3_IMMEDIATE), ADDR_MODE_3_WRITEBACK(ADDR_MODE_3_ADDRESS), BODY) \

#define DEFINE_LOAD_STORE_T_INSTRUCTION_SHIFTER_ARM(NAME, SHIFTER, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME, SHIFTER, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(-, ADDR_MODE_2_RM)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## U, SHIFTER, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(+, ADDR_MODE_2_RM)), BODY) \

#define DEFINE_LOAD_STORE_T_INSTRUCTION_ARM(NAME, BODY) \
	DEFINE_LOAD_STORE_T_INSTRUCTION_SHIFTER_ARM(NAME ## _LSL_, ADDR_MODE_2_LSL, BODY) \
	DEFINE_LOAD_STORE_T_INSTRUCTION_SHIFTER_ARM(NAME ## _LSR_, ADDR_MODE_2_LSR, BODY) \
	DEFINE_LOAD_STORE_T_INSTRUCTION_SHIFTER_ARM(NAME ## _ASR_, ADDR_MODE_2_ASR, BODY) \
	DEFINE_LOAD_STORE_T_INSTRUCTION_SHIFTER_ARM(NAME ## _ROR_, ADDR_MODE_2_ROR, BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## I, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(-, ADDR_MODE_2_IMMEDIATE)), BODY) \
	DEFINE_LOAD_STORE_INSTRUCTION_EX_ARM(NAME ## IU, ADDR_MODE_2_RN, ADDR_MODE_2_WRITEBACK(ADDR_MODE_2_INDEX(+, ADDR_MODE_2_IMMEDIATE)), BODY) \

#define ARM_MS_PRE \
	enum PrivilegeMode privilegeMode = cpu->privilegeMode; \
	ARMSetPrivilegeMode(cpu, MODE_SYSTEM);

#define ARM_MS_POST ARMSetPrivilegeMode(cpu, privilegeMode);

/*0.9.3
#define ARM_MS_PRE_store \
	enum PrivilegeMode privilegeMode = cpu->privilegeMode; \
	ARMSetPrivilegeMode(cpu, MODE_SYSTEM);

#define ARM_MS_PRE_load \
	enum PrivilegeMode privilegeMode; \
	if (!(rs & 0x8000) && rs) { \
		privilegeMode = cpu->privilegeMode; \
		ARMSetPrivilegeMode(cpu, MODE_SYSTEM); \
	}
*/
/*
宏定义：DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME, LS, WRITEBACK, S_PRE, S_POST, DIRECTION, POST_BODY)
替换文本：
static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rn = (opcode >> 16) & 0xF;
		int rs = opcode & 0x0000FFFF;
		uint32_t address = cpu->gprs[rn];
		S_PRE;
		address = cpu->memory. LS ## Multiple(cpu, address, rs, LSM_ ## DIRECTION, &currentCycles);
		S_POST;
		POST_BODY;
		WRITEBACK;
		cpu->cycles += currentCycles;
	}
*/
//数据加载/存储指令需要在ARM指令宏上扩展
#define DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME, LS, WRITEBACK, S_PRE, S_POST, DIRECTION, POST_BODY) \
	DEFINE_INSTRUCTION_ARM(NAME, \
		int rn = (opcode >> 16) & 0xF; \
		int rs = opcode & 0x0000FFFF; \
		uint32_t address = cpu->gprs[rn]; \
		S_PRE; \
		address = cpu->memory. LS ## Multiple(cpu, address, rs, LSM_ ## DIRECTION, &currentCycles); \
		S_POST; \
		POST_BODY; \
		WRITEBACK;)

//定义数据加载/存储（多个）指令的宏
#define DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_ARM(NAME, LS, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## DA,   LS,                               ,           ,            , DA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## DAW,  LS, ADDR_MODE_4_WRITEBACK_ ## NAME,           ,            , DA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## DB,   LS,                               ,           ,            , DB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## DBW,  LS, ADDR_MODE_4_WRITEBACK_ ## NAME,           ,            , DB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## IA,   LS,                               ,           ,            , IA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## IAW,  LS, ADDR_MODE_4_WRITEBACK_ ## NAME,           ,            , IA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## IB,   LS,                               ,           ,            , IB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## IBW,  LS, ADDR_MODE_4_WRITEBACK_ ## NAME,           ,            , IB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SDA,  LS,                               , ARM_MS_PRE, ARM_MS_POST, DA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SDAW, LS, ADDR_MODE_4_WRITEBACK_ ## NAME, ARM_MS_PRE, ARM_MS_POST, DA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SDB,  LS,                               , ARM_MS_PRE, ARM_MS_POST, DB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SDBW, LS, ADDR_MODE_4_WRITEBACK_ ## NAME, ARM_MS_PRE, ARM_MS_POST, DB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SIA,  LS,                               , ARM_MS_PRE, ARM_MS_POST, IA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SIAW, LS, ADDR_MODE_4_WRITEBACK_ ## NAME, ARM_MS_PRE, ARM_MS_POST, IA, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SIB,  LS,                               , ARM_MS_PRE, ARM_MS_POST, IB, POST_BODY) \
	DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_EX_ARM(NAME ## SIBW, LS, ADDR_MODE_4_WRITEBACK_ ## NAME, ARM_MS_PRE, ARM_MS_POST, IB, POST_BODY)

/* Begin ALU definitions，开始ALU指令定义

   Arithmetic: ADD ADC SUB SBC RSB RSC CMP CMN
      Logical: AND EOR ORR BIC TST TEQ
         Move: MOV MVN
*/
/* 原宏定义：DEFINE_ALU_INSTRUCTION_ARM(NAME, S_BODY, BODY)
	DEFINE_ALU_INSTRUCTION_ARM(ADD, ARM_ADDITION_S(n, cpu->shifterOperand, cpu->gprs[rd]),
			int32_t n = cpu->gprs[rn];
			cpu->gprs[rd] = n + cpu->shifterOperand;)
展开为：
**DEFINE_ALU_INSTRUCTION_EX_ARM(ADD ## _LSL, , _shiftLSL, int32_t n = cpu->gprs[rn]; cpu->gprs[rd] = n + cpu->shifterOperand;)**
static void _ARMInstruction ## ADD ## _LSL (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rn = (opcode >> 16) & 0xF;
		UNUSED(rn);
		_shiftLSL(cpu, opcode);
		int32_t n = cpu->gprs[rn]; 
		cpu->gprs[rd] = n + cpu->shifterOperand;;
		;
		if (rd == ARM_PC) {
			if (cpu->executionMode == MODE_ARM) {
				ARM_WRITE_PC;
			} else {
				THUMB_WRITE_PC;
			}
		}
		cpu->cycles += currentCycles;
	}
**DEFINE_ALU_INSTRUCTION_EX_ARM(ADD ## S_LSL, ARM_ADDITION_S(n, cpu->shifterOperand, cpu->gprs[rd]), _shiftLSL, 
								int32_t n = cpu->gprs[rn];
	                            cpu->gprs[rd] = n + cpu->shifterOperand;)**
static void _ARMInstruction ## ADD ## S_LSL (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rn = (opcode >> 16) & 0xF;
		UNUSED(rn);
		_shiftLSL(cpu, opcode);
		int32_t n = cpu->gprs[rn];
	    cpu->gprs[rd] = n + cpu->shifterOperand;;
		ARM_ADDITION_S(n, cpu->shifterOperand, cpu->gprs[rd]);;
		if (rd == ARM_PC) {
			if (cpu->executionMode == MODE_ARM) {
				ARM_WRITE_PC;
			} else {
				THUMB_WRITE_PC;
			}
		}
		cpu->cycles += currentCycles;
	}
	......
*/
DEFINE_ALU_INSTRUCTION_ARM(ADD, ARM_ADDITION_S(n, cpu->shifterOperand, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn];
	cpu->gprs[rd] = n + cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(ADC, ARM_ADDITION_S(n, cpu->shifterOperand, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn];
	cpu->gprs[rd] = n + cpu->shifterOperand + cpu->cpsr.c;)

DEFINE_ALU_INSTRUCTION_ARM(AND, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = cpu->gprs[rn] & cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(BIC, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = cpu->gprs[rn] & ~cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_S_ONLY_ARM(CMN, ARM_ADDITION_S(cpu->gprs[rn], cpu->shifterOperand, aluOut),
	int32_t aluOut = cpu->gprs[rn] + cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_S_ONLY_ARM(CMP, ARM_SUBTRACTION_S(cpu->gprs[rn], cpu->shifterOperand, aluOut),
	int32_t aluOut = cpu->gprs[rn] - cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(EOR, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = cpu->gprs[rn] ^ cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(MOV, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(MVN, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = ~cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(ORR, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, cpu->gprs[rd]),
	cpu->gprs[rd] = cpu->gprs[rn] | cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(RSB, ARM_SUBTRACTION_S(cpu->shifterOperand, n, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn];
	cpu->gprs[rd] = cpu->shifterOperand - n;)

DEFINE_ALU_INSTRUCTION_ARM(RSC, ARM_SUBTRACTION_S(cpu->shifterOperand, n, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn] + !cpu->cpsr.c;
	cpu->gprs[rd] = cpu->shifterOperand - n;)

DEFINE_ALU_INSTRUCTION_ARM(SBC, ARM_SUBTRACTION_S(n, shifterOperand, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn];
	int32_t shifterOperand = cpu->shifterOperand + !cpu->cpsr.c;
	cpu->gprs[rd] = n - shifterOperand;)

DEFINE_ALU_INSTRUCTION_ARM(SUB, ARM_SUBTRACTION_S(n, cpu->shifterOperand, cpu->gprs[rd]),
	int32_t n = cpu->gprs[rn];
	cpu->gprs[rd] = n - cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_S_ONLY_ARM(TEQ, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, aluOut),
	int32_t aluOut = cpu->gprs[rn] ^ cpu->shifterOperand;)

DEFINE_ALU_INSTRUCTION_S_ONLY_ARM(TST, ARM_NEUTRAL_S(cpu->gprs[rn], cpu->shifterOperand, aluOut),
	int32_t aluOut = cpu->gprs[rn] & cpu->shifterOperand;)

// End ALU definitions，结束ALU指令定义

/* Begin multiply definitions，开始乘法指令定义

   MUL: Multiply
   MLA: Multiply accumulate
   UMULL: Multiply unsigned long
   UMLAL: Multiply unsigned accumulate long
   SMULL: Multiply signed long
   SMLAL: Multiply signed accumulate long
*/
/*
原宏定义：DEFINE_MULTIPLY_INSTRUCTION_ARM(NAME, BODY, S_BODY)
DEFINE_MULTIPLY_INSTRUCTION_ARM(MLA, cpu->gprs[rdHi] = cpu->gprs[rm] * cpu->gprs[rs] + cpu->gprs[rd], ARM_NEUTRAL_S(, , cpu->gprs[rdHi]))
替换文本：
**DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(NAME, BODY, )**
static void _ARMInstruction ## MLA (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rdHi = (opcode >> 16) & 0xF;
		int rs = (opcode >> 8) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rdHi);
		ARM_WAIT_MUL(cpu->gprs[rs]);
		cpu->gprs[rdHi] = cpu->gprs[rm] * cpu->gprs[rs] + cpu->gprs[rd];
		;
		if (rd == ARM_PC) {
			ARM_WRITE_PC;
		}
		cpu->cycles += currentCycles;
	}
**DEFINE_MULTIPLY_INSTRUCTION_EX_ARM(MLA ## S, BODY, S_BODY)**
static void _ARMInstruction ## MLA ## S (struct ARMCore* cpu, uint32_t opcode) {
		int currentCycles = ARM_PREFETCH_CYCLES;
		int rd = (opcode >> 12) & 0xF;
		int rdHi = (opcode >> 16) & 0xF;
		int rs = (opcode >> 8) & 0xF;
		int rm = opcode & 0xF;
		UNUSED(rdHi);
		ARM_WAIT_MUL(cpu->gprs[rs]);
		cpu->gprs[rdHi] = cpu->gprs[rm] * cpu->gprs[rs] + cpu->gprs[rd];
		ARM_NEUTRAL_S(, , cpu->gprs[rdHi]);
		if (rd == ARM_PC) {
			ARM_WRITE_PC;
		}
		cpu->cycles += currentCycles;
	}
*/
DEFINE_MULTIPLY_INSTRUCTION_ARM(MLA, cpu->gprs[rdHi] = cpu->gprs[rm] * cpu->gprs[rs] + cpu->gprs[rd], ARM_NEUTRAL_S(, , cpu->gprs[rdHi]))
DEFINE_MULTIPLY_INSTRUCTION_ARM(MUL, cpu->gprs[rdHi] = cpu->gprs[rm] * cpu->gprs[rs], ARM_NEUTRAL_S(cpu->gprs[rm], cpu->gprs[rs], cpu->gprs[rdHi]))

DEFINE_MULTIPLY_INSTRUCTION_ARM(SMLAL,
	int64_t d = ((int64_t) cpu->gprs[rm]) * ((int64_t) cpu->gprs[rs]);
	int32_t dm = cpu->gprs[rd];
	int32_t dn = d;
	cpu->gprs[rd] = dm + dn;
	cpu->gprs[rdHi] = cpu->gprs[rdHi] + (d >> 32) + ARM_CARRY_FROM(dm, dn, cpu->gprs[rd]);,
	ARM_NEUTRAL_HI_S(cpu->gprs[rd], cpu->gprs[rdHi]))

DEFINE_MULTIPLY_INSTRUCTION_ARM(SMULL,
	int64_t d = ((int64_t) cpu->gprs[rm]) * ((int64_t) cpu->gprs[rs]);
	cpu->gprs[rd] = d;
	cpu->gprs[rdHi] = d >> 32;,
	ARM_NEUTRAL_HI_S(cpu->gprs[rd], cpu->gprs[rdHi]))

DEFINE_MULTIPLY_INSTRUCTION_ARM(UMLAL,
	uint64_t d = NO_EXTEND64(cpu->gprs[rm]) * NO_EXTEND64(cpu->gprs[rs]);
	int32_t dm = cpu->gprs[rd];
	int32_t dn = d;
	cpu->gprs[rd] = dm + dn;
	cpu->gprs[rdHi] = cpu->gprs[rdHi] + (d >> 32) + ARM_CARRY_FROM(dm, dn, cpu->gprs[rd]);,
	ARM_NEUTRAL_HI_S(cpu->gprs[rd], cpu->gprs[rdHi]))

DEFINE_MULTIPLY_INSTRUCTION_ARM(UMULL,
	uint64_t d = NO_EXTEND64(cpu->gprs[rm]) * NO_EXTEND64(cpu->gprs[rs]);
	cpu->gprs[rd] = d;
	cpu->gprs[rdHi] = d >> 32;,
	ARM_NEUTRAL_HI_S(cpu->gprs[rd], cpu->gprs[rdHi]))

// End multiply definitions，结束乘法指令定义

/* Begin load/store definitions，开始加载/存储指令定义

   LDR(LDRT) LDRH LDRB(LDRBT) LDRSH LDRSB
   STR(STRT) STRH STRB(STRBT)
   LDM
   STM
*/
DEFINE_LOAD_STORE_INSTRUCTION_ARM(LDR, cpu->gprs[rd] = cpu->memory.load32(cpu, address, &currentCycles); ARM_LOAD_POST_BODY;)
DEFINE_LOAD_STORE_INSTRUCTION_ARM(LDRB, cpu->gprs[rd] = cpu->memory.loadU8(cpu, address, &currentCycles); ARM_LOAD_POST_BODY;)
DEFINE_LOAD_STORE_MODE_3_INSTRUCTION_ARM(LDRH, cpu->gprs[rd] = cpu->memory.loadU16(cpu, address, &currentCycles); ARM_LOAD_POST_BODY;)
DEFINE_LOAD_STORE_MODE_3_INSTRUCTION_ARM(LDRSB, cpu->gprs[rd] = cpu->memory.load8(cpu, address, &currentCycles); ARM_LOAD_POST_BODY;)
DEFINE_LOAD_STORE_MODE_3_INSTRUCTION_ARM(LDRSH, cpu->gprs[rd] = cpu->memory.load16(cpu, address, &currentCycles); ARM_LOAD_POST_BODY;)
DEFINE_LOAD_STORE_INSTRUCTION_ARM(STR, cpu->memory.store32(cpu, address, cpu->gprs[rd], &currentCycles); ARM_STORE_POST_BODY;)
DEFINE_LOAD_STORE_INSTRUCTION_ARM(STRB, cpu->memory.store8(cpu, address, cpu->gprs[rd], &currentCycles); ARM_STORE_POST_BODY;)
DEFINE_LOAD_STORE_MODE_3_INSTRUCTION_ARM(STRH, cpu->memory.store16(cpu, address, cpu->gprs[rd], &currentCycles); ARM_STORE_POST_BODY;)

DEFINE_LOAD_STORE_T_INSTRUCTION_ARM(LDRBT,
	enum PrivilegeMode priv = cpu->privilegeMode;
	ARMSetPrivilegeMode(cpu, MODE_USER);
	cpu->gprs[rd] = cpu->memory.loadU8(cpu, address, &currentCycles);
	ARMSetPrivilegeMode(cpu, priv);
	ARM_LOAD_POST_BODY;)

DEFINE_LOAD_STORE_T_INSTRUCTION_ARM(LDRT,
	enum PrivilegeMode priv = cpu->privilegeMode;
	ARMSetPrivilegeMode(cpu, MODE_USER);
	cpu->gprs[rd] = cpu->memory.load32(cpu, address, &currentCycles);
	ARMSetPrivilegeMode(cpu, priv);
	ARM_LOAD_POST_BODY;)

DEFINE_LOAD_STORE_T_INSTRUCTION_ARM(STRBT,
	enum PrivilegeMode priv = cpu->privilegeMode;
	ARMSetPrivilegeMode(cpu, MODE_USER);
	cpu->memory.store32(cpu, address, cpu->gprs[rd], &currentCycles);
	ARMSetPrivilegeMode(cpu, priv);
	ARM_STORE_POST_BODY;)

DEFINE_LOAD_STORE_T_INSTRUCTION_ARM(STRT,
	enum PrivilegeMode priv = cpu->privilegeMode;
	ARMSetPrivilegeMode(cpu, MODE_USER);
	cpu->memory.store8(cpu, address, cpu->gprs[rd], &currentCycles);
	ARMSetPrivilegeMode(cpu, priv);
	ARM_STORE_POST_BODY;)

DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_ARM(LDM,
	load,
	++currentCycles;
	if (rs & 0x8000) {
		ARM_WRITE_PC;
	})

DEFINE_LOAD_STORE_MULTIPLE_INSTRUCTION_ARM(STM,
	store,
	currentCycles += cpu->memory.activeNonseqCycles32 - cpu->memory.activeSeqCycles32)

DEFINE_INSTRUCTION_ARM(SWP,
	int rm = opcode & 0xF;
	int rd = (opcode >> 12) & 0xF;
	int rn = (opcode >> 16) & 0xF;
	int32_t d = cpu->memory.load32(cpu, cpu->gprs[rn], &currentCycles);
	cpu->memory.store32(cpu, cpu->gprs[rn], cpu->gprs[rm], &currentCycles);
	cpu->gprs[rd] = d;)

DEFINE_INSTRUCTION_ARM(SWPB,
	int rm = opcode & 0xF;
	int rd = (opcode >> 12) & 0xF;
	int rn = (opcode >> 16) & 0xF;
	int32_t d = cpu->memory.loadU8(cpu, cpu->gprs[rn], &currentCycles);
	cpu->memory.store8(cpu, cpu->gprs[rn], cpu->gprs[rm], &currentCycles);
	cpu->gprs[rd] = d;)

// End load/store definitions，结束加载/存储指令定义

/* Begin branch definitions，开始分支指令定义

   B: Branch
   BL: Branch with link
   BX: Branch and exchange instruction set
*/
DEFINE_INSTRUCTION_ARM(B,
	int32_t offset = opcode << 8;
	offset >>= 6;
	cpu->gprs[ARM_PC] += offset;
	ARM_WRITE_PC;)

DEFINE_INSTRUCTION_ARM(BL,
	int32_t immediate = (opcode & 0x00FFFFFF) << 8;
	cpu->gprs[ARM_LR] = cpu->gprs[ARM_PC] - WORD_SIZE_ARM;
	cpu->gprs[ARM_PC] += immediate >> 6;
	ARM_WRITE_PC;)

DEFINE_INSTRUCTION_ARM(BX,
	int rm = opcode & 0x0000000F;
	_ARMSetMode(cpu, cpu->gprs[rm] & 0x00000001);
	cpu->gprs[ARM_PC] = cpu->gprs[rm] & 0xFFFFFFFE;
	if (cpu->executionMode == MODE_THUMB) {
		THUMB_WRITE_PC;
	} else {
		ARM_WRITE_PC;
	})

// End branch definitions，结束分支指令定义

/* Begin coprocessor definitions，协处理器指令定义

   CDP: Data operation
   LDC: Load
   STC: Store
   MRC: Move to ARM register from coprocessor
   MCR: Move to coprocessor from ARM register
*/
DEFINE_INSTRUCTION_ARM(CDP, ARM_STUB)
DEFINE_INSTRUCTION_ARM(LDC, ARM_STUB)
DEFINE_INSTRUCTION_ARM(STC, ARM_STUB)
DEFINE_INSTRUCTION_ARM(MCR, ARM_STUB)
DEFINE_INSTRUCTION_ARM(MRC, ARM_STUB)

/* Begin miscellaneous definitions，杂项（不好分类的指令）

   BKPT ILL
   MSR MSRR MSRI MSRRI
   MRS MRSR 
   SWI
*/
DEFINE_INSTRUCTION_ARM(BKPT, ARM_STUB) // Not strictly in ARMv4T, but here for convenience
DEFINE_INSTRUCTION_ARM(ILL, ARM_ILL) // Illegal opcode

DEFINE_INSTRUCTION_ARM(MSR,
	int c = opcode & 0x00010000;
	int f = opcode & 0x00080000;
	int32_t operand = cpu->gprs[opcode & 0x0000000F];
	int32_t mask = (c ? 0x000000FF : 0) | (f ? 0xFF000000 : 0);
	if (mask & PSR_USER_MASK) {
		cpu->cpsr.packed = (cpu->cpsr.packed & ~PSR_USER_MASK) | (operand & PSR_USER_MASK);
	}
	if (cpu->privilegeMode != MODE_USER && (mask & PSR_PRIV_MASK)) {
		ARMSetPrivilegeMode(cpu, (enum PrivilegeMode) ((operand & 0x0000000F) | 0x00000010));
		cpu->cpsr.packed = (cpu->cpsr.packed & ~PSR_PRIV_MASK) | (operand & PSR_PRIV_MASK);
	}
	_ARMReadCPSR(cpu);)

DEFINE_INSTRUCTION_ARM(MSRR,
	int c = opcode & 0x00010000;
	int f = opcode & 0x00080000;
	int32_t operand = cpu->gprs[opcode & 0x0000000F];
	int32_t mask = (c ? 0x000000FF : 0) | (f ? 0xFF000000 : 0);
	mask &= PSR_USER_MASK | PSR_PRIV_MASK | PSR_STATE_MASK;
	cpu->spsr.packed = (cpu->spsr.packed & ~mask) | (operand & mask);)

DEFINE_INSTRUCTION_ARM(MRS, \
	int rd = (opcode >> 12) & 0xF; \
	cpu->gprs[rd] = cpu->cpsr.packed;)

DEFINE_INSTRUCTION_ARM(MRSR, \
	int rd = (opcode >> 12) & 0xF; \
	cpu->gprs[rd] = cpu->spsr.packed;)

DEFINE_INSTRUCTION_ARM(MSRI,
	int c = opcode & 0x00010000;
	int f = opcode & 0x00080000;
	int rotate = (opcode & 0x00000F00) >> 7;
	int32_t operand = ARM_ROR(opcode & 0x000000FF, rotate);
	int32_t mask = (c ? 0x000000FF : 0) | (f ? 0xFF000000 : 0);
	if (mask & PSR_USER_MASK) {
		cpu->cpsr.packed = (cpu->cpsr.packed & ~PSR_USER_MASK) | (operand & PSR_USER_MASK);
	}
	if (cpu->privilegeMode != MODE_USER && (mask & PSR_PRIV_MASK)) {
		ARMSetPrivilegeMode(cpu, (enum PrivilegeMode) ((operand & 0x0000000F) | 0x00000010));
		cpu->cpsr.packed = (cpu->cpsr.packed & ~PSR_PRIV_MASK) | (operand & PSR_PRIV_MASK);
	}
	_ARMReadCPSR(cpu);)

DEFINE_INSTRUCTION_ARM(MSRRI,
	int c = opcode & 0x00010000;
	int f = opcode & 0x00080000;
	int rotate = (opcode & 0x00000F00) >> 7;
	int32_t operand = ARM_ROR(opcode & 0x000000FF, rotate);
	int32_t mask = (c ? 0x000000FF : 0) | (f ? 0xFF000000 : 0);
	mask &= PSR_USER_MASK | PSR_PRIV_MASK | PSR_STATE_MASK;
	cpu->spsr.packed = (cpu->spsr.packed & ~mask) | (operand & mask);)

DEFINE_INSTRUCTION_ARM(SWI, cpu->irqh.swi32(cpu, opcode & 0xFFFFFF))

const ARMInstruction _armTable[0x1000] = {
	DECLARE_ARM_EMITTER_BLOCK(_ARMInstruction)
};
