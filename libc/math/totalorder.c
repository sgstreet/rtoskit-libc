/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * totalorder.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * F.10.12.1: IEEE totalOrder. Flipping the magnitude bits of negative
 * encodings makes signed integer order the total order: -NaN < -infinity <
 * ... < -0 < +0 < ... < +infinity < +NaN, signaling NaNs nearer zero.
 */
int totalorder(const double *x, const double *y)
{
	int64_t a = (int64_t)__libc_double_bits(*x);
	int64_t b = (int64_t)__libc_double_bits(*y);

	a ^= (int64_t)((uint64_t)(a >> 63) >> 1);
	b ^= (int64_t)((uint64_t)(b >> 63) >> 1);

	return a <= b;
}
