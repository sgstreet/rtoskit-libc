/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_remainderf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.10, in float: r = x - n*y exactly, with n = x/y truncated (fmod)
 * or rounded to nearest, ties to even (remainder, remquo), and the low bits
 * of n. Long division on the significands (both scaled to [2^23, 2^24)), one
 * quotient bit per exponent step. A zero result has the sign of x.
 */
float __libc_remainderf(float x, float y, bool nearest, int *quotient)
{
	uint32_t bx = __libc_float_bits(x);
	uint32_t by = __libc_float_bits(y);
	uint32_t sign = bx & __LIBC_FLOAT_SIGN;
	bool negative_quotient = ((bx ^ by) & __LIBC_FLOAT_SIGN) != 0;
	int ex = __libc_float_exponent(bx);
	int ey = __libc_float_exponent(by);
	uint32_t mx = bx & __LIBC_FLOAT_FRACTION;
	uint32_t my = by & __LIBC_FLOAT_FRACTION;
	uint32_t q = 0;

	if (quotient != nullptr)
		*quotient = 0;
	if (isnan(x) || isnan(y))
		return x + y;
	if (ex == 0xff || (by << 1) == 0)
		return __libc_math_invalidf();
	if (ey == 0xff || (bx << 1) == 0)
		return x;

	if (ex != 0) {
		mx |= 1U << 23;
	} else {
		for (ex = 1; mx < (1U << 23); --ex)
			mx <<= 1;
	}
	if (ey != 0) {
		my |= 1U << 23;
	} else {
		for (ey = 1; my < (1U << 23); --ey)
			my <<= 1;
	}

	if (ex < ey) {
		/* |x| < |y|: n is 0, or 1 when |x| > |y|/2 (2|x| > |y| compares mx and my when ex is ey - 1) */
		if (!nearest || ex < ey - 1 || mx <= my)
			return x;
		if (quotient != nullptr)
			*quotient = negative_quotient ? -1 : 1;
		/* |x| - |y| is exact (Sterbenz) */
		return sign != 0 ? fabsf(y) - fabsf(x) : fabsf(x) - fabsf(y);
	}

	for (; ex > ey; --ex) {
		q <<= 1;
		if (mx >= my) {
			mx -= my;
			q |= 1;
		}
		mx <<= 1;
	}
	q <<= 1;
	if (mx >= my) {
		mx -= my;
		q |= 1;
	}

	if (nearest && (2 * mx > my || (2 * mx == my && (q & 1) != 0))) {
		mx = my - mx;
		++q;
		sign ^= __LIBC_FLOAT_SIGN;
	}
	if (quotient != nullptr)
		*quotient = negative_quotient ? -(int)(q & 0x7fffffff) : (int)(q & 0x7fffffff);
	if (mx == 0)
		return __libc_float_from(bx & __LIBC_FLOAT_SIGN);

	/* r = mx * 2^(ey - 150), exactly representable: normalise, or shift into the subnormal range */
	for (; mx < (1U << 23); --ey)
		mx <<= 1;
	if (ey <= 0) {
		mx >>= 1 - ey;
		ey = 0;
	}

	return __libc_float_from(sign | ((uint32_t)ey << 23) | (mx & __LIBC_FLOAT_FRACTION));
}
