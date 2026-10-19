/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fsqrt.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.14.5: sqrt(x) rounded to float once: the integer square root of
 * the significand scaled to an even exponent and 64 bits gives 32 bits and
 * the sticky bit.
 */
float fsqrt(double x)
{
	struct __libc_exact root;
	int exponent;
	uint64_t mantissa;
	bool inexact;
	uint64_t bits;

	if (isnan(x))
		return (float)(x + x);
	if (x < 0)
		return __libc_math_invalidf();
	if (x == 0 || isinf(x))
		return (float)x;

	mantissa = __libc_double_split(x, &exponent);
	if ((exponent & 1) != 0) {
		mantissa <<= 1;
		--exponent;
	}
	bits = __libc_isqrt(0, mantissa << 10, 32, &inexact);

	root = (struct __libc_exact){ .negative = false, .sticky = inexact, .exponent = (exponent - 10) / 2, .high = 0, .low = bits };
	return __libc_exact_to_float(&root);
}
