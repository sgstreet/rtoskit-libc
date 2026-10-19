/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * frexp.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.7: x = m * 2^exp with m in [0.5, 1); zero, infinities and NaNs as they are, exp 0 */
double frexp(double x, int *exp)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits);

	if (exponent == 0x7ff) {
		*exp = 0;
		return x + x;
	}
	if (exponent == 0) {
		if ((bits << 1) == 0) {
			*exp = 0;
			return x;
		}
		/* Subnormal: scale it into the normal range, exactly */
		bits = __libc_double_bits(x * 0x1p64);
		exponent = __libc_double_exponent(bits) - 64;
	}

	*exp = exponent - (__LIBC_DOUBLE_BIAS - 1);
	return __libc_double_from((bits & ~__LIBC_DOUBLE_EXPONENT) | ((uint64_t)(__LIBC_DOUBLE_BIAS - 1) << 52));
}
