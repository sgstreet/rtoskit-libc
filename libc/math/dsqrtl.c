/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * dsqrtl.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.14.5: long double is double (PLAN D7), so this is a correctly
 * rounded double square root: 59 bits of the integer square root of the
 * significand scaled by 2^64, and the sticky bit.
 */
double dsqrtl(long double x)
{
	struct __libc_exact root;
	int exponent;
	uint64_t mantissa;
	bool inexact;
	uint64_t bits;

	if (isnan(x))
		return x + x;
	if (x < 0)
		return __libc_math_invalid();
	if (x == 0 || isinf(x))
		return x;

	mantissa = __libc_double_split(x, &exponent);
	if ((exponent & 1) != 0) {
		mantissa <<= 1;
		--exponent;
	}
	bits = __libc_isqrt(mantissa, 0, 59, &inexact);

	root = (struct __libc_exact){ .negative = false, .sticky = inexact, .exponent = (exponent - 64) / 2, .high = 0, .low = bits };
	return __libc_exact_to_double(&root);
}
