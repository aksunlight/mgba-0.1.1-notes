/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef HLE_BIOS_H
#define HLE_BIOS_H

#include "util/common.h"

//A HLE BIOS is a BIOS emulated by software, by the emulator. As such, it doesn't need external files like other emulators.
//https://emulation.gametechwiki.com/index.php/High/Low_level_emulation
extern const uint8_t hleBios[];

#endif
