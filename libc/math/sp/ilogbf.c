/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ilogbf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.8: the unbiased exponent; zero, infinity and NaN are domain errors (invalid, F.10.3.8) */
int ilogbf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);

	if (exponent == 0xff || (bits << 1) == 0) {
		__libc_math_raise_invalid();
		return exponent != 0xff ? FP_ILOGB0 : (bits & __LIBC_FLOAT_FRACTION) != 0 ? FP_ILOGBNAN :
																					INT_MAX;
	}
	if (exponent == 0)
		exponent = __libc_float_exponent(__libc_float_bits(x * 0x1p32f)) - 32;

	return exponent - __LIBC_FLOAT_BIAS;
}
