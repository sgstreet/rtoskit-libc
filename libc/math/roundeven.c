/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * roundeven.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.8: halfway cases to the even integer; no inexact. With
 * exponent 0 the unit bit tested is the exponent's lowest bit, which is 1,
 * matching the integer part 1.
 */
double roundeven(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits) - __LIBC_DOUBLE_BIAS;
	uint64_t mask;
	uint64_t half;
	uint64_t rest;

	if (exponent >= 52)
		return exponent == 1024 ? x + x : x;
	if (exponent < -1 || (exponent == -1 && (bits & __LIBC_DOUBLE_FRACTION) == 0))
		return __libc_double_from(bits & __LIBC_DOUBLE_SIGN);
	if (exponent == -1)
		return __libc_double_from((bits & __LIBC_DOUBLE_SIGN) | __libc_double_bits(1.0));

	mask = __LIBC_DOUBLE_FRACTION >> exponent;
	half = (mask + 1) >> 1;
	rest = bits & mask;
	if (rest > half || (rest == half && (bits & (mask + 1)) != 0))
		bits += mask + 1;

	return __libc_double_from(bits & ~mask);
}
