/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nexttowardf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * x widened to the bits of a double, exactly, without a conversion helper:
 * a subnormal float is normal as a double.
 */
static uint64_t widen(uint32_t bits)
{
	uint64_t sign = (uint64_t)(bits & __LIBC_FLOAT_SIGN) << 32;
	int exponent = __libc_float_exponent(bits);
	uint32_t fraction = bits & __LIBC_FLOAT_FRACTION;

	if (exponent == 0xff)
		return sign | __LIBC_DOUBLE_EXPONENT | ((uint64_t)fraction << 29);
	if (exponent == 0) {
		if (fraction == 0)
			return sign;
		for (exponent = 1; fraction < (1U << 23); --exponent)
			fraction <<= 1;
		fraction &= __LIBC_FLOAT_FRACTION;
	}

	return sign | ((uint64_t)(exponent - __LIBC_FLOAT_BIAS + __LIBC_DOUBLE_BIAS) << 52) | ((uint64_t)fraction << 29);
}

/* The order of the values as a signed integer (-0 and +0 are equal) */
static int64_t order(uint64_t bits)
{
	int64_t magnitude = (int64_t)(bits & ~__LIBC_DOUBLE_SIGN);

	return (bits & __LIBC_DOUBLE_SIGN) != 0 ? -magnitude : magnitude;
}

/*
 * C23 7.12.11.4: nextafterf toward a long double (a double here), compared
 * through the bits so that nothing is computed in double (PLAN D56).
 */
float nexttowardf(float x, long double y)
{
	uint32_t bits = __libc_float_bits(x);
	uint64_t ybits = __libc_double_bits((double)y);
	int64_t from;
	int64_t to;
	float result;

	if (isnan(x))
		return x + x;
	if ((ybits & ~__LIBC_DOUBLE_SIGN) > __LIBC_DOUBLE_EXPONENT)
		return __libc_float_from((uint32_t)(ybits >> 32 & __LIBC_FLOAT_SIGN) | __LIBC_FLOAT_EXPONENT | __LIBC_FLOAT_QUIET | (uint32_t)((ybits & __LIBC_DOUBLE_FRACTION) >> 29));

	from = order(widen(bits));
	to = order(ybits);
	/* Equal: y converted to float, which is x except for the sign of a zero */
	if (from == to)
		return (bits << 1) == 0 ? __libc_float_from((uint32_t)(ybits >> 32) & __LIBC_FLOAT_SIGN) : x;

	if ((bits & ~__LIBC_FLOAT_SIGN) == 0)
		bits = (to < 0 ? __LIBC_FLOAT_SIGN : 0) | 1;
	else if ((to > from) == ((bits & __LIBC_FLOAT_SIGN) == 0))
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
