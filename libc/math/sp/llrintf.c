/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * llrintf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.5: lrintf for long long; the conversion from the bits (PLAN D56) */
long long llrintf(float x)
{
	float rounded = rintf(x);

	if (!(rounded >= -0x1p63f && rounded < 0x1p63f)) {
		__libc_math_raise_invalid();
		return LLONG_MIN;
	}

	return __libc_float_to_i64(rounded);
}
