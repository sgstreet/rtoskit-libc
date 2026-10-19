/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * round.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.6: halfway cases away from zero, whatever the rounding direction; no inexact */
double round(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits) - __LIBC_DOUBLE_BIAS;
	uint64_t mask;

	if (exponent >= 52)
		return exponent == 1024 ? x + x : x;
	if (exponent < 0)
		return __libc_double_from((bits & __LIBC_DOUBLE_SIGN) | (exponent == -1 ? __libc_double_bits(1.0) : 0));

	mask = __LIBC_DOUBLE_FRACTION >> exponent;
	bits += (mask + 1) >> 1;

	return __libc_double_from(bits & ~mask);
}
