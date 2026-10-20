/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_float_to_i64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* The significand shifted into place: no floating-point operation, so no exception (PLAN D56) */
int64_t __libc_float_to_i64(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);
	uint64_t significand = (bits & __LIBC_FLOAT_FRACTION) | (1U << 23);
	uint64_t magnitude;

	if (exponent < __LIBC_FLOAT_BIAS)
		return 0;
	magnitude = exponent >= __LIBC_FLOAT_BIAS + 23 ? significand << (exponent - __LIBC_FLOAT_BIAS - 23) : significand >> (__LIBC_FLOAT_BIAS + 23 - exponent);

	/* -2^63 negates to itself, INT64_MIN */
	return (int64_t)((bits >> 31) ? 0 - magnitude : magnitude);
}
