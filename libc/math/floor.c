/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * floor.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.2: toward -infinity. A fraction moving away from zero adds one unit at the
 * integer position; a carry may reach the exponent, which is still right.
 * No inexact (F.10.6).
 */
double floor(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits) - __LIBC_DOUBLE_BIAS;
	uint64_t mask;

	if (exponent >= 52)
		return exponent == 1024 ? x + x : x;
	if (exponent < 0) {
		if ((bits & ~__LIBC_DOUBLE_SIGN) == 0)
			return x;
		return (bits & __LIBC_DOUBLE_SIGN) != 0 ? -1.0 : __libc_double_from(bits & __LIBC_DOUBLE_SIGN);
	}

	mask = __LIBC_DOUBLE_FRACTION >> exponent;
	if ((bits & mask) == 0)
		return x;
	if ((bits & __LIBC_DOUBLE_SIGN) != 0)
		bits += mask + 1;

	return __libc_double_from(bits & ~mask);
}
