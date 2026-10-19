/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * totalorderf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* F.10.12.1 */
int totalorderf(const float *x, const float *y)
{
	int32_t a = (int32_t)__libc_float_bits(*x);
	int32_t b = (int32_t)__libc_float_bits(*y);

	a ^= (int32_t)((uint32_t)(a >> 31) >> 1);
	b ^= (int32_t)((uint32_t)(b >> 31) >> 1);

	return a <= b;
}
