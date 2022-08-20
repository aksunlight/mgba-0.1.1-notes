/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//宏函数
#ifndef MACROS_H
#define MACROS_H

#include "util/common.h"

#if defined(__PPC__) || defined(__POWERPC__)
#define LOAD_32(DEST, ADDR, ARR) { \
	uint32_t _addr = (ADDR); \
	void* _ptr = (ARR); \
	asm("lwbrx %0, %1, %2" : "=r"(DEST) : "b"(_ptr), "r"(_addr)); \
}

#define LOAD_16(DEST, ADDR, ARR) { \
	uint32_t _addr = (ADDR); \
	void* _ptr = (ARR); \
	asm("lhbrx %0, %1, %2" : "=r"(DEST) : "b"(_ptr), "r"(_addr)); \
}

#define STORE_32(SRC, ADDR, ARR) { \
	uint32_t _addr = (ADDR); \
	void* _ptr = (ARR); \
	asm("stwbrx %0, %1, %2" : : "r"(SRC), "b"(_ptr), "r"(_addr)); \
}

#define STORE_16(SRC, ADDR, ARR) { \
	uint32_t _addr = (ADDR); \
	void* _ptr = (ARR); \
	asm("sthbrx %0, %1, %2" : : "r"(SRC), "b"(_ptr), "r"(_addr)); \
}
#else
#define LOAD_32(DEST, ADDR, ARR) DEST = ((uint32_t*) ARR)[(ADDR) >> 2]
#define LOAD_16(DEST, ADDR, ARR) DEST = ((uint16_t*) ARR)[(ADDR) >> 1]
#define STORE_32(SRC, ADDR, ARR) ((uint32_t*) ARR)[(ADDR) >> 2] = SRC
#define STORE_16(SRC, ADDR, ARR) ((uint16_t*) ARR)[(ADDR) >> 1] = SRC
#endif

#define MAKE_MASK(START, END) (((1 << ((END) - (START))) - 1) << (START))
#define CHECK_BITS(SRC, START, END) ((SRC) & MAKE_MASK(START, END))
#define EXT_BITS(SRC, START, END) (((SRC) >> (START)) & ((1 << ((END) - (START))) - 1))
#define INS_BITS(SRC, START, END, BITS) (CLEAR_BITS(SRC, START, END) | (((BITS) << (START)) & MAKE_MASK(START, END)))
#define CLEAR_BITS(SRC, START, END) ((SRC) & ~MAKE_MASK(START, END))
#define FILL_BITS(SRC, START, END) ((SRC) | MAKE_MASK(START, END))

#define DECL_BITFIELD(NAME, TYPE) typedef TYPE NAME

#define DECL_BITS(TYPE, FIELD, START, SIZE) \
	__attribute__((unused)) static inline TYPE TYPE ## Is ## FIELD (TYPE src) { \
		return CHECK_BITS(src, (START), (START) + (SIZE)); \
	} \
	__attribute__((unused)) static inline TYPE TYPE ## Get ## FIELD (TYPE src) { \
		return EXT_BITS(src, (START), (START) + (SIZE)); \
	} \
	__attribute__((unused)) static inline TYPE TYPE ## Clear ## FIELD (TYPE src) { \
		return CLEAR_BITS(src, (START), (START) + (SIZE)); \
	} \
	__attribute__((unused)) static inline TYPE TYPE ## Fill ## FIELD (TYPE src) { \
		return FILL_BITS(src, (START), (START) + (SIZE)); \
	} \
	__attribute__((unused)) static inline TYPE TYPE ## Set ## FIELD (TYPE src, TYPE bits) { \
		return INS_BITS(src, (START), (START) + (SIZE), bits); \
	}

#define DECL_BIT(TYPE, FIELD, BIT) DECL_BITS(TYPE, FIELD, BIT, 1)

#define LIKELY(X) __builtin_expect(!!(X), 1)
#define UNLIKELY(X) __builtin_expect(!!(X), 0)
#endif
