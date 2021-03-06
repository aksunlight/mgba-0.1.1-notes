/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ARM_H
#define ARM_H

#include "util/common.h"

/*
ARM7TDMI芯片 -> ARMv4T指令集
在用户模式下ARM可见的寄存器有16个32位的寄存器（R0到R15）和一个当前程序状态寄存器CPSR
其中R15是程序计数器PC，R14用于存储子程序的返回地址LR，R13用于存储堆栈栈顶SP
*/
enum {
	ARM_SP = 13,	//SP: 栈指针寄存器，13号寄存器
	ARM_LR = 14,	//LR: 链接寄存器，存储于程序返回地址，14号寄存器
	ARM_PC = 15	//PC: 程序计数器，15号寄存器，因为指令是按字（4字节）对齐，故PC寄存器第0位和第1位为0
};

/*
ARM芯片有ARM状态和THUMB状态两种工作状态
ARM状态：32位，ARM状态执行字对齐的32位ARM指令
Thumb状态，16位，Thumb状态执行半字对齐的16位的Thumb指令
*/
enum ExecutionMode {
	MODE_ARM = 0,
	MODE_THUMB = 1
};

/*
ARM芯片有USER、FIQ、IRQ、SVC、ABT、SYS七种工作模式（新型芯片有Secure Monitor模式），除了USER模式其它6种处理器模式称为特权模式
特权模式下，程序可以访问所有的系统资源，也可以任意地进行处理器模式的切换，特权模式中，除系统模式外，其他5种模式又称为异常模式
大多数的用户程序运行在用户模式下，此时，应用程序不能够访问一些受操作系统保护的系统资源，应用程序也不能直接进行处理器模式的切换
用户模式下，当需要进行处理器模式切换时，应用程序可以产生异常处理，在异常处理中进行处理器模式的切换
ARM指令集中提供了两条产生异常的指令，通过这两条指令可以用软件的方法实现异常，其中一个就是软中断指令SWI
超级用户模式是CPU上电后默认模式，因此在该模式下主要用来做系统的初始化，软中断处理也在该模式下，当用户模式下的用户程序请求使用硬件资源时通过软件中断进入该模式
*/
enum PrivilegeMode {
	MODE_USER = 0x10,			//用户模式(PSR低5位为10000)，正常程序运行模式
	MODE_FIQ = 0x11,			//快速中断模式(PSR低5位为10001)，快速中断处理，用于高速数据传输
	MODE_IRQ = 0x12,			//外部中断模式(PSR低5位为10010)，普通中断处理
	MODE_SUPERVISOR = 0x13,			//超级用户模式(PSR低5位为10011)，提供操作系统使用的一种保护模式
	MODE_ABORT = 0x17,			//数据访问终止模式(PSR低5位为10111)，当数据或指令预取终止时进入该模式，用于虚拟存储和存储保护
	MODE_UNDEFINED = 0x1B,			//未定义指令终止模式(PSR低5位为11011)，当未定义的指令执行时进入该模式，用于支持通过软件仿真硬件的协处理
	MODE_SYSTEM = 0x1F			//系统模式(PSR低5位为11111)，用于运行特权级的操作系统任务
};

/*
ARM状态：32位，ARM状态执行字（ARM架构中字是32位4byte）对齐的32位ARM指令
Thumb状态，16位，Thumb状态执行半字（ARM架构中半字是16位2byte）对齐的16位的Thumb指令
*/
enum WordSize {
	WORD_SIZE_ARM = 4,
	WORD_SIZE_THUMB = 2
};

/*
异常发生时, ARM处理器会跳转到对应该异常的固定地址去执行异常处理程序, 这个固定的地址就是异常向量，即异常处理程序入口地址
注意：每一种异常运行在特定的处理器模式下，不同的工作模式可以发起模式特定的异常
*/
enum ExecutionVector {			//异常向量表
	BASE_RESET = 0x00000000,	//复位：处理器在工作时, 突然按下重启键,就会触发该异常
	BASE_UNDEF = 0x00000004,	//未定义指令：处理器无法识别指令的异常, 处理器执行的指令是有规范的, 如果尝试执行不符合要求的指令, 就会进入到该异常指令对应的地址中
	BASE_SWI = 0x00000008,		//软中断：软中断, 软件中需要去打断处理器工作, 可以使用软中断来执行
	BASE_PABT = 0x0000000C,		//预取指令失败：ARM在执行指令的过程中, 要先去预取指令准备执行, 如果预取指令失败, 就会产生该异常
	BASE_DABT = 0x00000010,		//读取数据失败
	BASE_IRQ = 0x00000018,		//普通中断
	BASE_FIQ = 0x0000001C		//快速中断：快速中断要比普通中断响应速度要快一些
};

/*
备份寄存器组
简单来说就是R8-R14寄存器对应若干个不同的物理寄存器，不同工作模式下使用不同物理寄存器
对于备份寄存器R8～R12来说，每个寄存器对应两个不同的物理寄存器
例如，当使用快速中断模式下的寄存器时，寄存器R8和寄存器R9分别记作R8_fiq、R9_fiq
当使用用户模式下的寄存器时，寄存器R8和寄存器R9分别记作R8_usr、R9_usr等
对于备份寄存器R13和R14来说，每个寄存器对应6个不同的物理寄存器
其中的一个是用户模式和系统模式共用的；另外的5个对应于其他5种处理器模式
参考：https://developer.arm.com/documentation/ddi0229/c/BGBJCJAE
*/
enum RegisterBank {			
	BANK_NONE = 0,		//对应user mode和sys mode下的备份寄存器组，这两种模式间的切换并不需要改变备份寄存器组
	BANK_FIQ = 1,
	BANK_IRQ = 2,
	BANK_SUPERVISOR = 3,
	BANK_ABORT = 4,
	BANK_UNDEFINED = 5
};

/*
LSM（load/store multiple）是存储加载多个的意思，ARM的LSM指令包括STM、LDM等等
ARM的多数据传输指令（STM、LDM）中，STM（store multiple register）是存储多个寄存器的意思，LDM（load multiple register）是加载多个寄存器的意思
存储多个寄存器意思是将多个寄存器的数据存入某一地址空间，加载多个寄存器的意思是将某一连续地址空间的数据存入多个寄存器
*/
enum LSMDirection {
	LSM_B = 1,
	LSM_D = 2,
	LSM_IA = 0,	//IA：increase after, 完成操作而后地址递增
	LSM_IB = 1,	//IB：increase before, 地址先增而后完成操作
	LSM_DA = 2,	//DA: decrease after, 完成操作而后地址递减
	LSM_DB = 3	//DB: decrease before, 地址先减而后完成操作
};

struct ARMCore;

/*
ARM芯片的程序状态寄存器PSR（Program State Register）有两个
一个是当前程序状态寄存器CPSR（Current Program State Register）
另一个是保存的程序状态寄存器SPSR（Saved Program State Register）
每一种处理器模式下都有一个专用的物理寄存器作为备份的程序状态寄存器即SPSR
当特定的异常发生时, 这个物理寄存器负责保存CPSR当前程序状态寄存器的内容
当异常处理程序返回时, 再将内容恢复到当前程序状态器中,继续向下执行原来程
PSR功能有：保存最近的逻辑或者算术操作的信息、控制中断时使能、设置处理器的工作模式等
*/
union PSR {
	struct {	//位域语法：type-specifier declarator(opt):constant-expression
				//位域用法：冒号后指定域的宽度（以位为单位），位域在表达式中的使用方式与同样基类型使用变量的方式完全相同，无论位域中有多少位
#if defined(__POWERPC__) || defined(__PPC__)
		////条件标志位(高4位 N,Z,C,V)
		unsigned n : 1;		//第31位Negative：负数 ? 1 : 0
		unsigned z : 1;		//第30位Zero：运算结果为0 ? 1 : 0
		unsigned c : 1;		//第29位Carry：进位标志，加/减法中产生了进/借位则为1
		unsigned v : 1;		//第28位Overflow：溢出标志，加减法中产生了溢出则为1
		unsigned : 20;		//第8位到第27位均不用考虑，并不重要
		//控制位(低8位 I,F,T,M[4:0]) 当发生异常时, 这些位的值将发生相应的变化, 在特权模式下, 也可以通过软件来修改这些位
		unsigned i : 1;		//第7位IRQ disable：I=1时IRQ禁止
		unsigned f : 1;		//第6位FIQ disable：f=1时FIQ禁止
		enum ExecutionMode t : 1;	//第5位Thumb state bit：t=0时处于ARM状态否则处于Thumb状态
		enum PrivilegeMode priv : 5;	//第0位到第4位Mode bit：这5位组合控制处理器处于什么工作模式
#else
		enum PrivilegeMode priv : 5;
		enum ExecutionMode t : 1;
		unsigned f : 1;
		unsigned i : 1;
		unsigned : 20;
		unsigned v : 1;
		unsigned c : 1;
		unsigned z : 1;
		unsigned n : 1;
#endif
	};

	int32_t packed;
};

struct ARMMemory {		//ARM内存
	int32_t (*load32)(struct ARMCore*, uint32_t address, int* cycleCounter);
	int16_t (*load16)(struct ARMCore*, uint32_t address, int* cycleCounter);
	uint16_t (*loadU16)(struct ARMCore*, uint32_t address, int* cycleCounter);
	int8_t (*load8)(struct ARMCore*, uint32_t address, int* cycleCounter);
	uint8_t (*loadU8)(struct ARMCore*, uint32_t address, int* cycleCounter);

	void (*store32)(struct ARMCore*, uint32_t address, int32_t value, int* cycleCounter);
	void (*store16)(struct ARMCore*, uint32_t address, int16_t value, int* cycleCounter);
	void (*store8)(struct ARMCore*, uint32_t address, int8_t value, int* cycleCounter);

	uint32_t (*loadMultiple)(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);
	uint32_t (*storeMultiple)(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);

	uint32_t* activeRegion;		//当前指令/事件基地址
	uint32_t activeMask;		//当前指令/事件偏移地址
	uint32_t activeSeqCycles32;	//序列周期数
	uint32_t activeSeqCycles16;
	uint32_t activeNonseqCycles32;	//非序列周期数
	uint32_t activeNonseqCycles16;
	uint32_t activeUncachedCycles32;	//未缓存周期数
	uint32_t activeUncachedCycles16;
	void (*setActiveRegion)(struct ARMCore*, uint32_t address);		//设置当前指令/事件基地址
};

struct ARMInterruptHandler {	//ARM中断处理程序
	void (*reset)(struct ARMCore* cpu);		//重启复位
	void (*processEvents)(struct ARMCore* cpu);	
	void (*swi16)(struct ARMCore* cpu, int immediate);	//16位软中断
	void (*swi32)(struct ARMCore* cpu, int immediate);	//32位软中断
	void (*hitIllegal)(struct ARMCore* cpu, uint32_t opcode);	//非法指令？
	void (*readCPSR)(struct ARMCore* cpu);	//读当前程序状态寄存器	

	void (*hitStub)(struct ARMCore* cpu, uint32_t opcode);
};

struct ARMComponent {		//ARM进程？
	uint32_t id;
	void (*init)(struct ARMCore* cpu, struct ARMComponent* component);
	void (*deinit)(struct ARMComponent* component);
};

struct ARMCore {		//ARM核心
	int32_t gprs[16];	//当前16个32位的寄存器（R0到R15）
	union PSR cpsr;		//当前程序状态寄存器
	union PSR spsr;		//保存的程序状态寄存器

	int32_t cycles;		//时钟周期数？指令周期数？机器周期数？
	int32_t nextEvent;	//直到完成下一条指令/事件的所有时钟周期数？指令周期数？机器周期数？
	int halted;		//停止

	int32_t bankedRegisters[6][7];	//备份寄存器组，存储不同工作模式下每种工作模式的bankedRegisters，
	int32_t bankedSPSRs[6];		//SPSR寄存器，存储不同工作模式每种工作模式下的SPSR

	int32_t shifterOperand;		//数据计算类指令的第二操作数的值
	int32_t shifterCarryOut;	//数据计算类指令的执行某一工作模式？

	uint32_t prefetch;			//预取指令
	enum ExecutionMode executionMode;	//当前工作状态
	enum PrivilegeMode privilegeMode;	//当前工作模式

	struct ARMMemory memory;		//内存	
	struct ARMInterruptHandler irqh;	//中断句柄

	struct ARMComponent* master;	//主进程？

	int numComponents;		//其他进程？
	struct ARMComponent** components;	
};

void ARMInit(struct ARMCore* cpu);		//初始化
void ARMDeinit(struct ARMCore* cpu);	//另一种初始化方式
void ARMSetComponents(struct ARMCore* cpu, struct ARMComponent* master, int extra, struct ARMComponent** extras);	//设置进程？

void ARMReset(struct ARMCore* cpu);		//重置
void ARMSetPrivilegeMode(struct ARMCore*, enum PrivilegeMode);	//设置工作模式
void ARMRaiseIRQ(struct ARMCore*);		//拉起普通中断
void ARMRaiseSWI(struct ARMCore*);		//拉起软中断

void ARMRun(struct ARMCore* cpu);		//单步运行
void ARMRunLoop(struct ARMCore* cpu);	//循环运行

#endif
