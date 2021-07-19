/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "hle-bios.h"

#include "gba-memory.h"

const uint8_t hleBios[SIZE_BIOS] = {
  0x06, 0x00, 0x00, 0xea, 0xfe, 0xff, 0xff, 0xea, 0x05, 0x00, 0x00, 0xea,
  0xfe, 0xff, 0xff, 0xea, 0xfe, 0xff, 0xff, 0xea, 0x00, 0x00, 0xa0, 0xe1,
  0x25, 0x00, 0x00, 0xea, 0xfe, 0xff, 0xff, 0xea, 0x02, 0xf3, 0xa0, 0xe3,
  0x00, 0x00, 0x5d, 0xe3, 0x01, 0xd3, 0xa0, 0x03, 0x20, 0xd0, 0x4d, 0x02,
  0x00, 0x58, 0x2d, 0xe9, 0x02, 0xb0, 0x5e, 0xe5, 0x80, 0xc0, 0xa0, 0xe3,
  0x0b, 0xb1, 0x9c, 0xe7, 0x00, 0x00, 0x5b, 0xe3, 0x00, 0xc0, 0x4f, 0xe1,
  0x00, 0x10, 0x2d, 0xe9, 0x80, 0xc0, 0x0c, 0xe2, 0x1f, 0xc0, 0x8c, 0xe3,
  0x0c, 0xf0, 0x29, 0xe1, 0x00, 0x40, 0x2d, 0xe9, 0x0f, 0xe0, 0xa0, 0xe1,
  0x1b, 0xff, 0x2f, 0x11, 0x00, 0x40, 0xbd, 0xe8, 0x93, 0xf0, 0x29, 0xe3,
  0x00, 0x10, 0xbd, 0xe8, 0x0c, 0xf0, 0x69, 0xe1, 0x00, 0x58, 0xbd, 0xe8,
  0x0e, 0xf0, 0xb0, 0xe1, 0x01, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xd4, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xac, 0x01, 0x00, 0x00,
  0x0f, 0x50, 0x2d, 0xe9, 0x01, 0x03, 0xa0, 0xe3, 0x00, 0xe0, 0x8f, 0xe2,
  0x04, 0xf0, 0x10, 0xe5, 0x0f, 0x50, 0xbd, 0xe8, 0x04, 0xf0, 0x5e, 0xe2,
  0x01, 0x00, 0xa0, 0xe3, 0x01, 0x10, 0xa0, 0xe3, 0x0c, 0x40, 0x2d, 0xe9,
  0x01, 0xc3, 0xa0, 0xe3, 0x00, 0x00, 0x50, 0xe3, 0x00, 0x00, 0xa0, 0xe3,
  0x01, 0x20, 0xa0, 0xe3, 0x03, 0x00, 0x00, 0x0a, 0xb8, 0x30, 0x5c, 0xe1,
  0x01, 0x30, 0xc3, 0xe1, 0xb8, 0x30, 0x4c, 0xe1, 0x01, 0x03, 0xcc, 0xe5,
  0x08, 0x02, 0xcc, 0xe5, 0xb8, 0x30, 0x5c, 0xe1, 0x01, 0x30, 0x13, 0xe0,
  0x01, 0x30, 0x23, 0x10, 0xb8, 0x30, 0x4c, 0x11, 0x08, 0x22, 0xcc, 0xe5,
  0xf7, 0xff, 0xff, 0x0a, 0x0c, 0x80, 0xbd, 0xe8, 0x00, 0x40, 0x2d, 0xe9,
  0x02, 0x36, 0xa0, 0xe1, 0x01, 0x04, 0x12, 0xe3, 0x0f, 0x00, 0x00, 0x0a,
  0x01, 0x03, 0x12, 0xe3, 0x05, 0x00, 0x00, 0x0a, 0x23, 0x35, 0x81, 0xe0,
  0x04, 0x00, 0xb0, 0xe8, 0x03, 0x00, 0x51, 0xe1, 0x04, 0x00, 0xa1, 0xb8,
  0xfc, 0xff, 0xff, 0xba, 0x16, 0x00, 0x00, 0xea, 0x01, 0x00, 0xc0, 0xe3,
  0x01, 0x10, 0xc1, 0xe3, 0xa3, 0x35, 0x81, 0xe0, 0xb0, 0x20, 0xd0, 0xe1,
  0x03, 0x00, 0x51, 0xe1, 0xb2, 0x20, 0xc1, 0xb0, 0xfc, 0xff, 0xff, 0xba,
  0x0e, 0x00, 0x00, 0xea, 0x01, 0x03, 0x12, 0xe3, 0x05, 0x00, 0x00, 0x0a,
  0x23, 0x35, 0x81, 0xe0, 0x03, 0x00, 0x51, 0xe1, 0x04, 0x00, 0xb0, 0xb8,
  0x04, 0x00, 0xa1, 0xb8, 0xfb, 0xff, 0xff, 0xba, 0x06, 0x00, 0x00, 0xea,
  0xa3, 0x35, 0x81, 0xe0, 0x01, 0x00, 0xc0, 0xe3, 0x01, 0x10, 0xc1, 0xe3,
  0x03, 0x00, 0x51, 0xe1, 0xb2, 0x20, 0xd0, 0xb0, 0xb2, 0x20, 0xc1, 0xb0,
  0xfb, 0xff, 0xff, 0xba, 0x00, 0x80, 0xbd, 0xe8, 0xf0, 0x47, 0x2d, 0xe9,
  0x01, 0x04, 0x12, 0xe3, 0x02, 0x36, 0xa0, 0xe1, 0x23, 0x25, 0x81, 0xe0,
  0x0b, 0x00, 0x00, 0x0a, 0x00, 0x30, 0x90, 0xe5, 0x03, 0x40, 0xa0, 0xe1,
  0x03, 0x50, 0xa0, 0xe1, 0x03, 0x60, 0xa0, 0xe1, 0x03, 0x70, 0xa0, 0xe1,
  0x03, 0x80, 0xa0, 0xe1, 0x03, 0x90, 0xa0, 0xe1, 0x03, 0xa0, 0xa0, 0xe1,
  0x02, 0x00, 0x51, 0xe1, 0xf8, 0x07, 0xa1, 0xb8, 0xfc, 0xff, 0xff, 0xba,
  0x03, 0x00, 0x00, 0xea, 0x02, 0x00, 0x51, 0xe1, 0xf8, 0x07, 0xb0, 0xb8,
  0xf8, 0x07, 0xa1, 0xb8, 0xfb, 0xff, 0xff, 0xba, 0xf0, 0x87, 0xbd, 0xe8
};
