/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nextafter.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.11.3, F.10.8.3: the next representable value after x toward y;
 * y when they are equal. Stepping out of the finite range raises overflow,
 * a subnormal or zero result underflow, each with inexact.
 */
double nextafter(double x, double y)
{
	uint64_t bits = __libc_double_bits(x);
	double result;

	if (isnan(x) || isnan(y))
		return x + y;
	if (x == y)
		return y;

	if ((bits & ~__LIBC_DOUBLE_SIGN) == 0)
		bits = (__libc_double_bits(y) & __LIBC_DOUBLE_SIGN) | 1;
	else if ((y > x) == ((bits & __LIBC_DOUBLE_SIGN) == 0))
		++bits;
	else
		--bits;

	result = __libc_double_from(bits);
	if ((bits & __LIBC_DOUBLE_EXPONENT) == __LIBC_DOUBLE_EXPONENT)
		__libc_math_raise_overflow();
	else if ((bits & __LIBC_DOUBLE_EXPONENT) == 0)
		__libc_math_raise_underflow();

	return result;
}
