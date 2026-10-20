/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * lrintf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.5, F.10.6: outside the range of long the result is unspecified
 * (LONG_MIN) and invalid is raised; otherwise inexact when the value changed (rintf raises it).
 */
long lrintf(float x)
{
	float limit = sizeof(long) == 8 ? 0x1p63f : 0x1p31f;
	float rounded = rintf(x);

	if (!(rounded >= -limit && rounded < limit)) {
		__libc_math_raise_invalid();
		return LONG_MIN;
	}

	return sizeof(long) == 8 ? (long)__libc_float_to_i64(rounded) : (long)rounded;
}
