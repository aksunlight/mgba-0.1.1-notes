/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GBA_GPIO_H
#define GBA_GPIO_H

#include "util/common.h"

/*
 * GPIO = GBA Cart I/O Port (GPIO) = GBA 卡带 I/O 端口
 * 4bit General Purpose I/O Port (GPIO) - contained in the ROM-chip，4个通用型I/O端口 - 包含在ROM芯片中
 * 
 * GPIO端口即通用输入输出端口，就是芯片的一些引脚。作为输入端口时，可以透过读取某个寄存器/暂存器确定引脚电位的高低
 * 作为输出端口时，可以通过写入寄存器/暂存器来让这个引脚输出高电位或者低电位，从而达到控制外围设备的目的
 */

#define IS_GPIO_REGISTER(reg) ((reg) == GPIO_REG_DATA || (reg) == GPIO_REG_DIRECTION || (reg) == GPIO_REG_CONTROL)

/*
 * GPIO可连接的外围设备
 *
 * Connection Examples(连接举例)
 * GPIO       | Boktai  | Wario
 * Bit Pin    | RTC SOL | GYR RBL
 * -----------+---------+---------
 * 0   ROM.1  | SCK CLK | RES -
 * 1   ROM.2  | SIO RST | CLK -
 * 2   ROM.21 | CS  -   | DTA -
 * 3   ROM.22 | -   FLG | -   MOT
 * -----------+---------+---------
 * IRQ ROM.43 | IRQ -   | -   -
 */
enum GPIODevice {
	GPIO_NONE = 0,
	GPIO_RTC = 1,
	GPIO_RUMBLE = 2,
	GPIO_LIGHT_SENSOR = 4,
	GPIO_GYRO = 8,
	GPIO_TILT = 16
};

/*
 * GPIO控制寄存器
 *
 * 80000C4h - I/O Port Data (selectable W or R/W)
 *   bit0-3  Data Bits 0..3 (0=Low, 1=High)
 *   bit4-15 not used (0)
 * 
 * 80000C6h - I/O Port Direction (for above Data Port) (selectable W or R/W)
 *   bit0-3  Direction for Data Port Bits 0..3 (0=In, 1=Out)
 *   bit4-15 not used (0)
 * 
 * 80000C8h - I/O Port Control (selectable W or R/W)
 *   bit0    Register 80000C4h..80000C8h Control (0=Write-Only, 1=Read/Write)
 *   bit1-15 not used (0)
 * 
 * In write-only mode, reads return 00h (or possible other data, if the rom contains non-zero data at that location).
 */
enum GPIORegister {
	GPIO_REG_DATA = 0xC4,
	GPIO_REG_DIRECTION = 0xC6,
	GPIO_REG_CONTROL = 0xC8
};

enum GPIODirection {
	GPIO_WRITE_ONLY = 0,
	GPIO_READ_WRITE = 1
};

union RTCControl {
	struct {
		unsigned : 3;
		unsigned minIRQ : 1;
		unsigned : 2;
		unsigned hour24 : 1;
		unsigned poweroff : 1;
	};
	uint8_t packed;
};

enum RTCCommand {
	RTC_RESET = 0,
	RTC_DATETIME = 2,
	RTC_FORCE_IRQ = 3,
	RTC_CONTROL = 4,
	RTC_TIME = 6
};

union RTCCommandData {
	struct {
		unsigned magic : 4;
		enum RTCCommand command : 3;
		unsigned reading : 1;
	};
	uint8_t packed;
};

struct GBARTC {
	int bytesRemaining;
	int transferStep;
	int bitsRead;
	int bits;
	int commandActive;
	union RTCCommandData command;
	union RTCControl control;
	uint8_t time[7];
} __attribute__((packed));

struct GBARumble {
	void (*setRumble)(struct GBARumble*, int enable);
};

struct GBACartridgeGPIO {
	struct GBA* p;
	int gpioDevices;
	enum GPIODirection readWrite;
	uint16_t* gpioBase;

	union {
		struct {
			unsigned p0 : 1;
			unsigned p1 : 1;
			unsigned p2 : 1;
			unsigned p3 : 1;
		};
		uint16_t pinState;
	};

	union {
		struct {
			unsigned dir0 : 1;
			unsigned dir1 : 1;
			unsigned dir2 : 1;
			unsigned dir3 : 1;			
		};
		uint16_t direction;
	};

	struct GBARTC rtc;

	uint16_t gyroSample;
	bool gyroEdge;
};

void GBAGPIOInit(struct GBACartridgeGPIO* gpio, uint16_t* gpioBase);
void GBAGPIOWrite(struct GBACartridgeGPIO* gpio, uint32_t address, uint16_t value);

void GBAGPIOInitRTC(struct GBACartridgeGPIO* gpio);

void GBAGPIOInitGyro(struct GBACartridgeGPIO* gpio);

void GBAGPIOInitRumble(struct GBACartridgeGPIO* gpio);

struct GBASerializedState;
void GBAGPIOSerialize(struct GBACartridgeGPIO* gpio, struct GBASerializedState* state);
void GBAGPIODeserialize(struct GBACartridgeGPIO* gpio, struct GBASerializedState* state);

#endif
