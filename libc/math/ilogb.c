/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ilogb.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.6.8, F.10.3: the exponent as an integer; for zero (FP_ILOGB0), a NaN
 * (FP_ILOGBNAN) or an infinity (INT_MAX) the result is out of range and invalid is
 * raised.
 */
int ilogb(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits);

	if (exponent == 0x7ff || (bits << 1) == 0) {
		__libc_math_raise_invalid();
		return exponent != 0x7ff ? FP_ILOGB0 : (bits & __LIBC_DOUBLE_FRACTION) != 0 ? FP_ILOGBNAN :
																					  INT_MAX;
	}
	if (exponent == 0)
		exponent = __libc_double_exponent(__libc_double_bits(x * 0x1p64)) - 64;

	return exponent - __LIBC_DOUBLE_BIAS;
}
