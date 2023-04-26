/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GBA_BIOS_H
#define GBA_BIOS_H

#include "util/common.h"

#include "arm.h"

/*
 * The BIOS includes several System Call Functions which can be accessed by SWI instructions.
 * When invoking SWIs from inside of ARM state specify SWI NN*10000h, instead of SWI NN as in THUMB state.
 */
void GBASwi16(struct ARMCore* cpu, int immediate);
void GBASwi32(struct ARMCore* cpu, int immediate);

uint32_t GBAChecksum(uint32_t* memory, size_t size);
const uint32_t GBA_BIOS_CHECKSUM;
const uint32_t GBA_DS_BIOS_CHECKSUM;

#endif
