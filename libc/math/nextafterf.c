/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nextafterf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.11.3, F.10.8.3 */
float nextafterf(float x, float y)
{
	uint32_t bits = __libc_float_bits(x);
	float result;

	if (isnan(x) || isnan(y))
		return (float)(x + y);
	if (x == y)
		return (float)y;

	if ((bits & ~__LIBC_FLOAT_SIGN) == 0)
		bits = (signbit(y) ? __LIBC_FLOAT_SIGN : 0) | 1;
	else if ((y > x) == ((bits & __LIBC_FLOAT_SIGN) == 0))
		++bits;
	else
		--bits;

	result = __libc_float_from(bits);
	if ((bits & __LIBC_FLOAT_EXPONENT) == __LIBC_FLOAT_EXPONENT)
		__libc_math_raise_overflow();
	else if ((bits & __LIBC_FLOAT_EXPONENT) == 0)
		__libc_math_raise_underflow();

	return result;
}
