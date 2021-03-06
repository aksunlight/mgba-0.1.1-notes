/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef PATCH_H
#define PATCH_H

#include "util/common.h"

struct VFile;

struct Patch {
	struct VFile* vf;

	size_t (*outputSize)(struct Patch* patch, size_t inSize);
	bool (*applyPatch)(struct Patch* patch, void* out, size_t outSize);
};

bool loadPatch(struct VFile* vf, struct Patch* patch);

#endif
