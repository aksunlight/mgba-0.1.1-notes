/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GBA_MEMORY_H
#define GBA_MEMORY_H

#include "util/common.h"

#include "arm.h"
#include "macros.h"

#include "gba-gpio.h"
#include "gba-savedata.h"

/*
GBA Memory Map

General Internal Memory
  00000000-00003FFF   BIOS - System ROM         (16 KBytes)
  00004000-01FFFFFF   Not used
  02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes) 2 Wait
  02040000-02FFFFFF   Not used
  03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
  03008000-03FFFFFF   Not used
  04000000-040003FE   I/O Registers
  04000400-04FFFFFF   Not used

Internal Display Memory
  05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
  05000400-05FFFFFF   Not used
  06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
  06018000-06FFFFFF   Not used
  07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
  07000400-07FFFFFF   Not used

External Memory (Game Pak)
  08000000-09FFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0
  0A000000-0BFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1
  0C000000-0DFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2
  0E000000-0E00FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width
  0E010000-0FFFFFFF   Not used

Unused Memory Area
  10000000-FFFFFFFF   Not used (upper 4bits of address bus unused)
*/

enum GBAMemoryRegion {
	REGION_BIOS = 0x0,
	REGION_WORKING_RAM = 0x2,
	REGION_WORKING_IRAM = 0x3,
	REGION_IO = 0x4,
	REGION_PALETTE_RAM = 0x5,
	REGION_VRAM = 0x6,
	REGION_OAM = 0x7,
	REGION_CART0 = 0x8,
	REGION_CART0_EX = 0x9,
	REGION_CART1 = 0xA,
	REGION_CART1_EX = 0xB,
	REGION_CART2 = 0xC,
	REGION_CART2_EX = 0xD,
	REGION_CART_SRAM = 0xE,
	REGION_CART_SRAM_MIRROR = 0xF
};

enum GBAMemoryBase {
	BASE_BIOS = 0x00000000,
	BASE_WORKING_RAM = 0x02000000,
	BASE_WORKING_IRAM = 0x03000000,
	BASE_IO = 0x04000000,
	BASE_PALETTE_RAM = 0x05000000,
	BASE_VRAM = 0x06000000,
	BASE_OAM = 0x07000000,
	BASE_CART0 = 0x08000000,
	BASE_CART0_EX = 0x09000000,
	BASE_CART1 = 0x0A000000,
	BASE_CART1_EX = 0x0B000000,
	BASE_CART2 = 0x0C000000,
	BASE_CART2_EX = 0x0D000000,
	BASE_CART_SRAM = 0x0E000000,
	BASE_CART_SRAM_MIRROR = 0x0F000000
};

enum {
	SIZE_BIOS = 0x00004000,
	SIZE_WORKING_RAM = 0x00040000,
	SIZE_WORKING_IRAM = 0x00008000,
	SIZE_IO = 0x00000400,
	SIZE_PALETTE_RAM = 0x00000400,
	SIZE_VRAM = 0x00018000,
	SIZE_OAM = 0x00000400,
	SIZE_CART0 = 0x02000000,
	SIZE_CART1 = 0x02000000,
	SIZE_CART2 = 0x02000000,
	SIZE_CART_SRAM = 0x00008000,
	SIZE_CART_FLASH512 = 0x00010000,
	SIZE_CART_FLASH1M = 0x00020000,
	SIZE_CART_EEPROM = 0x00002000
};

//通过地址高8位确定地址所在区域（BIOS、WRAM、IWRAM、I/O Registers...），低24位确定偏移量
enum {
	OFFSET_MASK = 0x00FFFFFF,
	BASE_OFFSET = 24
};

enum DMAControl {
	DMA_INCREMENT = 0,
	DMA_DECREMENT = 1,
	DMA_FIXED = 2,
	DMA_INCREMENT_RELOAD = 3
};

enum DMATiming {
	DMA_TIMING_NOW = 0,
	DMA_TIMING_VBLANK = 1,
	DMA_TIMING_HBLANK = 2,
	DMA_TIMING_CUSTOM = 3
};


DECL_BITFIELD(GBADMARegister, uint16_t);
DECL_BITS(GBADMARegister, DestControl, 5, 2);
DECL_BITS(GBADMARegister, SrcControl, 7, 2);
DECL_BIT(GBADMARegister, Repeat, 9);
DECL_BIT(GBADMARegister, Width, 10);
DECL_BIT(GBADMARegister, DRQ, 11);
DECL_BITS(GBADMARegister, Timing, 12, 2);
DECL_BIT(GBADMARegister, DoIRQ, 14);
DECL_BIT(GBADMARegister, Enable, 15);

struct GBADMA {
	GBADMARegister reg;

	uint32_t source;
	uint32_t dest;
	int32_t count;
	uint32_t nextSource;
	uint32_t nextDest;
	int32_t nextCount;
	int32_t nextEvent;
};

struct GBAMemory {
	uint32_t* bios;				//0x00000000-0x00003FFF   BIOS - System ROM (16 KBytes)
	uint32_t* wram;				//0x02000000-0x0203FFFF   WRAM - On-board Work RAM (256 KBytes) 2 Wait
	uint32_t* iwram;			//0x03000000-0x03007FFF   WRAM - On-chip Work RAM (32 KBytes)
	uint32_t* rom;				//0x08000000-0x0FFFFFFF   External Memory (Game Pak)
	uint16_t io[SIZE_IO >> 1];	//0x04000000-040003FE   I/O Registers

	struct GBACartridgeGPIO gpio;
	struct GBASavedata savedata;
	size_t romSize;
	uint16_t romID;
	int fullBios;

	/*
	 *The ARM7TDMI also has four different types of cycles, on which
	 *the CPU clock may stall for a different amount of time.
	 *How long each stall is depends on which region of memory is being accessed.
	 *The GBA refers to these stalls as “wait states”.
	*/
	char waitstatesSeq32[256];
	char waitstatesSeq16[256];
	char waitstatesNonseq32[256];
	char waitstatesNonseq16[256];
	char waitstatesPrefetchSeq32[16];
	char waitstatesPrefetchSeq16[16];
	char waitstatesPrefetchNonseq32[16];
	char waitstatesPrefetchNonseq16[16];
	int activeRegion;		//GBA内存活动区域，BIOS、WRAM、IWRAM、I/O Registers...，取值为0x0、0x2、0x3...
	uint32_t biosPrefetch;

	struct GBADMA dma[4];	//The GBA includes four DMA channels，GBA包含4个DMA通道
	int activeDMA;
	int32_t nextDMA;
	int32_t eventDiff;
};

void GBAMemoryInit(struct GBA* gba);
void GBAMemoryDeinit(struct GBA* gba);

void GBAMemoryReset(struct GBA* gba);

int32_t GBALoad32(struct ARMCore* cpu, uint32_t address, int* cycleCounter);
int16_t GBALoad16(struct ARMCore* cpu, uint32_t address, int* cycleCounter);
uint16_t GBALoadU16(struct ARMCore* cpu, uint32_t address, int* cycleCounter);
int8_t GBALoad8(struct ARMCore* cpu, uint32_t address, int* cycleCounter);
uint8_t GBALoadU8(struct ARMCore* cpu, uint32_t address, int* cycleCounter);

void GBAStore32(struct ARMCore* cpu, uint32_t address, int32_t value, int* cycleCounter);
void GBAStore16(struct ARMCore* cpu, uint32_t address, int16_t value, int* cycleCounter);
void GBAStore8(struct ARMCore* cpu, uint32_t address, int8_t value, int* cycleCounter);

uint32_t GBALoadMultiple(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);
uint32_t GBAStoreMultiple(struct ARMCore*, uint32_t baseAddress, int mask, enum LSMDirection direction, int* cycleCounter);

void GBAAdjustWaitstates(struct GBA* gba, uint16_t parameters);

void GBAMemoryWriteDMASAD(struct GBA* gba, int dma, uint32_t address);
void GBAMemoryWriteDMADAD(struct GBA* gba, int dma, uint32_t address);
void GBAMemoryWriteDMACNT_LO(struct GBA* gba, int dma, uint16_t count);
uint16_t GBAMemoryWriteDMACNT_HI(struct GBA* gba, int dma, uint16_t control);

void GBAMemoryScheduleDMA(struct GBA* gba, int number, struct GBADMA* info);
void GBAMemoryRunHblankDMAs(struct GBA* gba, int32_t cycles);
void GBAMemoryRunVblankDMAs(struct GBA* gba, int32_t cycles);
void GBAMemoryUpdateDMAs(struct GBA* gba, int32_t cycles);
int32_t GBAMemoryRunDMAs(struct GBA* gba, int32_t cycles);

struct GBASerializedState;
void GBAMemorySerialize(struct GBAMemory* memory, struct GBASerializedState* state);
void GBAMemoryDeserialize(struct GBAMemory* memory, struct GBASerializedState* state);

#endif
