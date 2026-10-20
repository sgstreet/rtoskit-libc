/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * roundf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.6: half an ulp of the integer part added to the magnitude, then the fraction cleared (ties away from zero) */
float roundf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits) - __LIBC_FLOAT_BIAS;
	uint32_t mask;

	if (exponent >= 23)
		return exponent == 128 ? x + x : x;
	if (exponent < 0)
		return __libc_float_from((bits & __LIBC_FLOAT_SIGN) | (exponent == -1 ? __libc_float_bits(1.0f) : 0));

	mask = __LIBC_FLOAT_FRACTION >> exponent;
	bits += (mask + 1) >> 1;

	return __libc_float_from(bits & ~mask);
}
