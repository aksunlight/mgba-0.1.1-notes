/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ARM_H
#define ARM_H

#include "util/common.h"

/*
ARM7TDMI（处理器内核/处理器/CPU） -> ARMv4T（体系结构版本/架构）

ARM7TDMI支持2种工作状态（支持32位指令的ARM状态和支持16位指令的THUMB状态），使用三级流水线

ARM7TDMI支持7种工作模式，它门分别为：用户模式（user）、系统模式（sys）、管理/访管模式（svc）
快速中断模式（fiq）、外部中断模式（irq）、数据访问中止模式（abt）、未定义指令中止模式（und）
除了user和system模式外，其它5种工作模式都是异常模式（比如Supervisor call模式是软件中断，FIQ和IRQ是硬件中断，Abort和Undef模式是异常）

ARMv6架构以前的处理器有37个物理寄存器，包括31个通用32位寄存器和、1个当前程序状态寄存器CPSR、5个保存程序状态寄存器SPSR，它门都是32位寄存器
ARM状态下，R14一般作为子程序链接寄存器（LR）用存储子程序调用后的返回地址（PC-4），R13一般作为堆栈指针寄存器（SP）指向堆栈栈顶，R15（PC）指向正在被取值的地址（oldPC+8）

                  ARM状态下的通用寄存器和程序计数器
System & User    FIQ    Supervisor    Abort      IRQ     Undefined
      R0         R0         R0         R0        R0         R0
      R1         R1         R1         R1        R1         R1
      R2         R2         R2         R2        R2         R2
      R3         R3         R3         R3        R3         R3
      R4         R4         R4         R4        R4         R4
      R5         R5         R5         R5        R5         R5
      R6         R6         R6         R6        R6         R6
      R7         R7         R7         R7        R7         R7
      R8        *R8_fiq     R8         R8        R8         R8
      R9        *R9_fiq     R9         R9        R9         R9
     R10        *R10_fiq    R10        R10       R10        R10
     R11        *R11_fiq    R11        R11       R11        R11
     R12        *R12_fiq    R12        R12       R12        R12
     R13        *R13_fiq   *R13_svc   *R13_abt  *R13_irq   *R13_und
     R14        *R14_fiq   *R14_svc   *R14_abt  *R14_irq   *R14_und
     R15(PC)     R15(PC)    R15(PC)    R15(PC)   R15(PC)    R15(PC)
                      ARM状态下的程序状态寄存器	
     CPSR        CPSR       CPSR       CPSR      CPSR       CPSR
                *SPSR_fiq  *SPSR_svc  *SPSR_abt *SPSR_irq  *SPSR_und

                  THUMB状态下的通用寄存器和程序计数器
System & User    FIQ    Supervisor    Abort      IRQ     Undefined
      R0         R0         R0         R0        R0         R0
      R1         R1         R1         R1        R1         R1
      R2         R2         R2         R2        R2         R2
      R3         R3         R3         R3        R3         R3
      R4         R4         R4         R4        R4         R4
      R5         R5         R5         R5        R5         R5
      R6         R6         R6         R6        R6         R6
      R7         R7         R7         R7        R7         R7
      SP        *SP_fiq    *SP_svc    *SP_abt   *SP_irq    *SP_und
      LR        *LR_fiq    *LR_svc    *LR_abt   *LR_irq    *LR_und
      PC         PC         PC         PC        PC         PC
                      THUMB状态下的程序状态寄存器	
     CPSR        CPSR       CPSR       CPSR      CPSR       CPSR
                *SPSR_fiq  *SPSR_svc  *SPSR_abt *SPSR_irq  *SPSR_und
*/
enum {
	ARM_SP = 13,    //SP: 栈指针寄存器，13号寄存器
	ARM_LR = 14,    //LR: 链接寄存器，存储子程序调用后的返回地址，14号寄存器
	ARM_PC = 15     //PC: 程序计数器，15号寄存器，因为指令是按字（4字节）对齐，故PC寄存器第0位和第1位为0
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
ARM芯片有USER、FIQ、IRQ、SVC、ABT、SYS、UND七种工作模式，除了USER模式其它6种处理器模式称为特权模式
特权模式下，程序可以访问所有的系统资源，也可以任意地进行处理器模式的切换，特权模式中，除系统模式外，
其他5种模式又称为异常模式，比如IRQ和FIQ模式是硬件中断，SVC模式是软件中断，ABT和UND模式是异常
大多数的用户程序运行在用户模式下，此时，应用程序不能够访问一些受操作系统保护的系统资源，应用程序也不能直接进行处理器模式的切换
用户模式下，当需要进行处理器模式切换时，应用程序可以产生异常处理，在异常处理中进行处理器模式的切换
ARM指令集中提供了两条产生异常的指令，通过这两条指令可以用软件的方法实现异常，其中一个就是软中断指令SWI
访管模式是CPU上电后默认模式，因此在该模式下主要用来做系统的初始化，软中断处理也在该模式下，当用户模式下的用户程序请求使用硬件资源时通过软件中断进入该模式

ARM CPSR寄存器格式（v4T架构）：
N Z C V    Unsed    I F T Mode
31-28      27-8     7 6 5 4-0

CPSR寄存器条件标志位的意义如下：
N：负数，改变标志位的最后的ALU操作产生负数结果（32位结果的最高位是1）
Z：零，改变标志位的最后的ALU操作产生0结果（32位结果的每一位都是0）
C：进位/借位，改变标志位的最后的ALU操作产生到符号位的进位
V：溢出，改变标志位的最后的ALU操作产生到符号位的溢出
*/
enum PrivilegeMode {    //特权模式下PSR寄存器设置
	MODE_USER = 0x10,           //用户模式(PSR低5位为10000)，正常程序运行模式
	MODE_FIQ = 0x11,            //快速中断模式(PSR低5位为10001)，快速中断处理，用于高速数据传输
	MODE_IRQ = 0x12,            //外部中断模式(PSR低5位为10010)，普通中断处理
	MODE_SUPERVISOR = 0x13,     //访管模式(PSR低5位为10011)，提供操作系统使用的一种保护模式，执行软中断时进入该模式
	MODE_ABORT = 0x17,          //数据访问终止模式(PSR低5位为10111)，当数据或指令预取终止时进入该模式，用于虚拟存储和存储保护
	MODE_UNDEFINED = 0x1B,      //未定义指令终止模式(PSR低5位为11011)，当未定义的指令执行时进入该模式，用于支持通过软件仿真硬件的协处理
	MODE_SYSTEM = 0x1F          //系统模式(PSR低5位为11111)，用于运行特权级的操作系统任务
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
异常发生时, 处理器会中断现在的操作去执行异常处理程序, 它通过查找异常向量表找到异常对应的处理程序入口地址从而执行异常处理
处理器的异常可以分为：硬件中断、软件中断和异常, 硬件中断一般是由外部的硬件引起的的事件, 比如串口来数据、键盘敲击
软件中断是通过在程序执行中执行的中断指令引起的中断, 如ARM中的SWI软中断指令, 软中断指令一般用于操作系统的系统调用入口
异常是由于CPU内部在运行过程中引起的事件，比如指令预取错误、数据中止、未定义指令等，异常事件一般由操作系统接管
注意：ARMv4T架构中每个异常对应异常向量表中的4个字节的空间，其中存放了一个跳转指令或者向PC指令赋值的数据访问指令，而不是异常处理程序入口地址

异常向量表：
地址            异常       进入模式        优先级（6最低）
0x00000000     复位        管理模式            1
0x00000004   未定义指令     未定义模式          6
0x00000008    软件中断      管理模式            6
0x0000000C   预取指令中止   中止模式            5
0x00000010    数据中止      中止模式           2
0x00000014     保留         保留              未使用
0x00000018     IRQ         IRQ               4
0x0000001C     FIQ         FIQ                3

ARM处理器对异常的响应过程：
1.保存处理器当前状态, 包括中断屏蔽位和各条件标志位, 它通过将CPSR的内容保存到将要执行的异常对应的SPSR寄存器中实现
2.设置CPSR中的相应位, 包括：进入ARM状态、设置mode位使处理器进入相应异常执行模式、设置CPSR的I位, 禁止IRQ中断, 当进入IRQ模式时, 禁止FIQ中断
3.将放回地址（pc-4）保存到寄存器lr_mode
4.将pc设置成该异常的异常向量地址, 从而跳转到相应的异常处理程序处执行（通过异常向量地址找到异常向量表中对应的异常, 
通过执行该异常对应的4个字节的空间中存储的指令跳转到真正的异常处理程序）

ARM处理器从异常中返回过程：
1.通用寄存器的恢复（使用前首先将其压栈保存，在退出异常服务程序前将其出栈恢复）
2.恢复被中断的程序处理器的状态，即将SPSR_mode寄存器的内容复制到当前程序状态寄存器CPSR中
3.返回到异常发生的下一条指令处执行，即将ir_mode寄存器的内容复制到程序计数器PC中
*/
enum ExecutionVector {          //异常向量表/中断向量表
	BASE_RESET = 0x00000000,    //复位：处理器在工作时, 突然按下重启键, 就会触发该异常
	BASE_UNDEF = 0x00000004,    //未定义指令：处理器无法识别指令的异常, 就会触发该异常
	BASE_SWI = 0x00000008,      //软件中断：软中断, 一般用于操作系统的系统调用入口
	BASE_PABT = 0x0000000C,     //预取指令失败：ARM在执行指令的过程中, 要先去预取指令准备执行, 如果预取指令失败, 就会产生该异常
	BASE_DABT = 0x00000010,     //读取数据失败
	BASE_IRQ = 0x00000018,      //普通中断IRQ
	BASE_FIQ = 0x0000001C       //快速中断FIQ
};

/*
寄存器组，各个模式下使用的寄存器集合略有不同，有些模式会用到备份寄存器
*/
enum RegisterBank {			
	BANK_NONE = 0,
	BANK_FIQ = 1,
	BANK_IRQ = 2,
	BANK_SUPERVISOR = 3,
	BANK_ABORT = 4,
	BANK_UNDEFINED = 5
};

/*
LSM（load/store multiple）是存储加载多个的意思，ARM的LSM指令包括批量数据存储STM和批量数据加载LDM
批量数据存储STM是将多个寄存器的数据存入某一连续地址空间，批量数据加载LDM是将某一连续地址空间的数据存入多个寄存器
LSM指令格式为：LDM(或STM) {条件}{类型} 基址寄存器{!}, 寄存器列表{^}
{!}为可选后缀，若选用，则当数据传送完毕后将最后的地址写入基址寄存器，否则基址寄存器的内容不变
{^}为可选后缀，当指令为LDM且寄存器列表中包含R15，选用该后缀则除了正常数据传送还将SPSR复制到CPSR

LDM/STM指令都将按照低地址对应低寄存器、高地址对应高寄存器的方式进行数据存取
STMIB r10, {r0, r1, r2, r3}    ;r0->[r10+4], r1->[r10+8], r2->[r10+12], r3->[r10+16]
STMIB r10, {r3, r1, r0, r3}    ;r0->[r10+4], r1->[r10+8], r2->[r10+12], r3->[r10+16]
*/
enum LSMDirection {//加载向量表, 所有批量加载/存储指令必须指明加载类型, 即加载方向
	LSM_B = 1,
	LSM_D = 2,
	LSM_IA = 0,    //IA：increase after, 每次传送后地址加4
	LSM_IB = 1,    //IB：increase before, 每次传送前地址加4
	LSM_DA = 2,    //DA: decrease after, 每次传送后地址减4
	LSM_DB = 3     //DB: decrease before, 每次传送前地址减4
};

struct ARMCore;

/*
ARM芯片的程序状态寄存器PSR（Program State Register）有两个
一个是当前程序状态寄存器CPSR（Current Program State Register）
另一个是保存的程序状态寄存器SPSR（Saved Program State Register）
除user、sys外的5种处理器模式都有一个专用的物理寄存器作为备份的程序状态寄存器
当异常发生时, 这个物理寄存器负责保存CPSR当前程序状态寄存器的内容
当异常处理程序返回时, 再将内容恢复到当前程序状态器中, 恢复现场后继续执行原来程序
ARM程序状态寄存器格式（v4T架构）：
N Z C V    Unsed    I F T Mode
31-28      27-8     7 6 5 4-0

CPSR寄存器条件标志位的意义如下：The N, Z, C, and V (Negative, Zero, Carry and oVerflow)
N：Is set to bit 31 of the result of the instruction. **If this result is regarded as a two's complement 
signed integer**, then N = 1 if the result is negative and N = 0 if it is positive or zero.
Z：Is set to 1 if the result of the instruction is zero (this often indicates an equal result from a 
comparison), and to 0 otherwise.
C：For an addition, including the comparison instruction CMN, C is set to 1 if the 
addition produced a carry (that is, an unsigned overflow), and to 0 otherwise.
For a subtraction, including the comparison instruction CMP, C is set to 0 if the 
subtraction produced a borrow (that is, an unsigned underflow), and to 1 otherwise.
For non-addition/subtractions that incorporate a shift operation, C is set to the last bit 
shifted out of the value by the shifter.  移位操作会改变C标志位，C标志位被设为移位器移位出的最后一位的值！
具体来说，移位操作使得移位器产生进位carry-out，这个进位一般是移位器移出的最后一位的值，这个值被一些指令用来设置C标志位
For other non-addition/subtractions, C is normally left unchanged (but see the 
individual instruction descriptions for any special cases).
V：For an addition or subtraction, V is set to 1 if signed overflow occurred, regarding the 
operands and result as two's complement signed integers.
For non-addition/subtractions, V is normally left unchanged (but see the individual 
instruction descriptions for any special cases).
*/
union PSR {
	struct {	//位域语法：type-specifier declarator(opt):constant-expression
				//位域用法：冒号后指定域的宽度（以位为单位），位域在表达式中的使用方式与同样基类型使用变量的方式完全相同，无论位域中有多少位
#if defined(__POWERPC__) || defined(__PPC__)	//PowerPC架构，大端模式
		//条件标志位(高4位 N,Z,C,V)
		unsigned n : 1;					//第31位Negative：负数 ? 1 : 0
		unsigned z : 1;					//第30位Zero：运算结果为0 ? 1 : 0
		unsigned c : 1;					//第29位Carry：进位标志，加/减法中产生了进/借位则为1
		unsigned v : 1;					//第28位Overflow：溢出标志，加减法中产生了溢出则为1
		unsigned : 20;					//第8位到第27位均不用考虑，并不重要
		//控制位(低8位 I,F,T,M[4:0]) 当发生异常时, 这些位的值将发生相应的变化, 在特权模式下, 也可以通过软件来修改这些位
		unsigned i : 1;					//第7位IRQ disable：I=1时IRQ禁止
		unsigned f : 1;					//第6位FIQ disable：f=1时FIQ禁止
		enum ExecutionMode t : 1;		//第5位Thumb state bit：t=0时处于ARM状态否则处于Thumb状态
		enum PrivilegeMode priv : 5;	//第0位到第4位Mode bit：这5位组合控制处理器处于什么工作模式
#else											//小端模式
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

struct ARMMemory {
	//对应字数据加载指令LDR
	int32_t (*load32)(struct ARMCore*, uint32_t address, int* cycleCounter);
	//对应半字数据加载指令LDRH, 目标寄存器高16位清零
	int16_t (*load16)(struct ARMCore*, uint32_t address, int* cycleCounter);
	//对应字节数据加载指令LDRB, 目标寄存器高24位清零
	int8_t (*load8)(struct ARMCore*, uint32_t address, int* cycleCounter);
	//对应有符号半字数据加载指令LDRSH, 目标寄存器高16位做符号位扩展
	uint16_t (*loadU16)(struct ARMCore*, uint32_t address, int* cycleCounter);
	//对应有符号字节数据加载指令LDRSB, 目标寄存器高24位做符号位扩展
	uint8_t (*loadU8)(struct ARMCore*, uint32_t address, int* cycleCounter);
	//对应字数据存储指令STR
	void (*store32)(struct ARMCore*, uint32_t address, int32_t value, int* cycleCounter);
	//对应半字数据存储指令STRH
	void (*store16)(struct ARMCore*, uint32_t address, int16_t value, int* cycleCounter);
	//对应字节数据存储指令STRB
	void (*store8)(struct ARMCore*, uint32_t address, int8_t value, int* cycleCounter);
	//对应批量数据加载指令LDM, 
	uint32_t (*loadMultiple)(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);
	//对应批量数据存储指令STM
	uint32_t (*storeMultiple)(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);

	uint32_t* activeRegion;				//当前指令/事件基地址
	uint32_t activeMask;				//当前指令/事件偏移地址
	uint32_t activeSeqCycles32;			//序列周期数
	uint32_t activeSeqCycles16;
	uint32_t activeNonseqCycles32;		//非序列周期数
	uint32_t activeNonseqCycles16;
	uint32_t activeUncachedCycles32;	//未缓存周期数
	uint32_t activeUncachedCycles16;

	//设置当前指令/事件基地址
	void (*setActiveRegion)(struct ARMCore*, uint32_t address);
};

struct ARMInterruptHandler {									//ARM中断处理程序
	void (*reset)(struct ARMCore* cpu);							//复位异常
	void (*processEvents)(struct ARMCore* cpu);					//FIQ、IRQ硬件中断
	void (*swi16)(struct ARMCore* cpu, int immediate);			//16位软中断
	void (*swi32)(struct ARMCore* cpu, int immediate);			//32位软中断
	void (*hitIllegal)(struct ARMCore* cpu, uint32_t opcode);	//未定义指令异常
	void (*readCPSR)(struct ARMCore* cpu);						//读当前程序状态寄存器	

	void (*hitStub)(struct ARMCore* cpu, uint32_t opcode);		//指令预取中止异常、数据中止异常
};

struct ARMComponent {        //ARM进程？
	uint32_t id;
	void (*init)(struct ARMCore* cpu, struct ARMComponent* component);
	void (*deinit)(struct ARMComponent* component);
};

struct ARMCore {		//ARM核心
	int32_t gprs[16];	//当前16个32位的寄存器（R0到R15）
	union PSR cpsr;		//当前程序状态寄存器
	union PSR spsr;		//保存的程序状态寄存器

	int32_t cycles;		//时钟周期数
	int32_t nextEvent;	//直到完成下一条指令/事件的所有时钟周期数？指令周期数？机器周期数？
	int halted;			//停止

	int32_t bankedRegisters[6][7];		//备份寄存器组，存储不同工作模式下每种工作模式的bankedRegisters，
	int32_t bankedSPSRs[6];				//SPSR寄存器组，存储不同工作模式下每种工作模式下的SPSR

	int32_t shifterOperand;				//数据处理指令第二源操作数的值
	int32_t shifterCarryOut;			//数据处理指令带来的C标志位的新值

	uint32_t prefetch;					//预取指令
	enum ExecutionMode executionMode;	//当前工作状态
	enum PrivilegeMode privilegeMode;	//当前工作模式

	struct ARMMemory memory;			//内存	
	struct ARMInterruptHandler irqh;	//中断句柄

	struct ARMComponent* master;		//主进程？

	int numComponents;					//其他进程？
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
