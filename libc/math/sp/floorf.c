/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * floorf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.2: the fraction bits cleared, rounding down the negative values (no inexact, F.10.6.2) */
float floorf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits) - __LIBC_FLOAT_BIAS;
	uint32_t mask;

	if (exponent >= 23)
		return exponent == 128 ? x + x : x;
	if (exponent < 0) {
		if ((bits & ~__LIBC_FLOAT_SIGN) == 0)
			return x;
		return (bits & __LIBC_FLOAT_SIGN) != 0 ? -1.0f : 0.0f;
	}

	mask = __LIBC_FLOAT_FRACTION >> exponent;
	if ((bits & mask) == 0)
		return x;
	if ((bits & __LIBC_FLOAT_SIGN) != 0)
		bits += mask + 1;

	return __libc_float_from(bits & ~mask);
}
