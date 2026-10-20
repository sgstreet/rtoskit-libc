/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_double_to_i64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/math.h"

/* The significand shifted into place: no floating-point operation, so no exception (PLAN D55) */
int64_t __libc_double_to_i64(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits);
	uint64_t significand = (bits & __LIBC_DOUBLE_FRACTION) | (1ULL << 52);
	uint64_t magnitude;

	if (exponent < __LIBC_DOUBLE_BIAS)
		return 0;
	magnitude = exponent >= __LIBC_DOUBLE_BIAS + 52 ? significand << (exponent - __LIBC_DOUBLE_BIAS - 52) : significand >> (__LIBC_DOUBLE_BIAS + 52 - exponent);

	/* -2^63 negates to itself, INT64_MIN */
	return (int64_t)((bits >> 63) ? 0 - magnitude : magnitude);
}
