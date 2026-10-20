/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * roundevenf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.9: to nearest, ties to the even integer */
float roundevenf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits) - __LIBC_FLOAT_BIAS;
	uint32_t mask;
	uint32_t half;
	uint32_t rest;

	if (exponent >= 23)
		return exponent == 128 ? x + x : x;
	if (exponent < -1 || (exponent == -1 && (bits & __LIBC_FLOAT_FRACTION) == 0))
		return __libc_float_from(bits & __LIBC_FLOAT_SIGN);
	if (exponent == -1)
		return __libc_float_from((bits & __LIBC_FLOAT_SIGN) | __libc_float_bits(1.0f));

	mask = __LIBC_FLOAT_FRACTION >> exponent;
	half = (mask + 1) >> 1;
	rest = bits & mask;
	if (rest > half || (rest == half && (bits & (mask + 1)) != 0))
		bits += mask + 1;

	return __libc_float_from(bits & ~mask);
}
