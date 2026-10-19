/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_remainder.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.10: r = x - n*y, exactly, with n = x/y truncated (fmod) or
 * rounded to nearest, ties to even (remainder, remquo), and the low bits of
 * n. Long division on the significands (both scaled to [2^52, 2^53)), one
 * quotient bit per exponent step. A zero result has the sign of x.
 */
double __libc_remainder(double x, double y, bool nearest, int *quotient)
{
	uint64_t bx = __libc_double_bits(x);
	uint64_t by = __libc_double_bits(y);
	uint64_t sign = bx & __LIBC_DOUBLE_SIGN;
	bool negative_quotient = ((bx ^ by) & __LIBC_DOUBLE_SIGN) != 0;
	int ex = __libc_double_exponent(bx);
	int ey = __libc_double_exponent(by);
	uint64_t mx = bx & __LIBC_DOUBLE_FRACTION;
	uint64_t my = by & __LIBC_DOUBLE_FRACTION;
	uint32_t q = 0;

	if (quotient != nullptr)
		*quotient = 0;
	if (isnan(x) || isnan(y))
		return x + y;
	if (ex == 0x7ff || (by << 1) == 0)
		return __libc_math_invalid();
	if (ey == 0x7ff || (bx << 1) == 0)
		return x;

	if (ex != 0) {
		mx |= 1ULL << 52;
	} else {
		for (ex = 1; mx < (1ULL << 52); --ex)
			mx <<= 1;
	}
	if (ey != 0) {
		my |= 1ULL << 52;
	} else {
		for (ey = 1; my < (1ULL << 52); --ey)
			my <<= 1;
	}

	if (ex < ey) {
		/* |x| < |y|: n is 0, or 1 when |x| > |y|/2 (2|x| > |y| compares mx and my when ex is ey - 1) */
		if (!nearest || ex < ey - 1 || mx <= my)
			return x;
		if (quotient != nullptr)
			*quotient = negative_quotient ? -1 : 1;
		/* |x| - |y| is exact (Sterbenz) */
		return sign != 0 ? fabs(y) - fabs(x) : fabs(x) - fabs(y);
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
		sign ^= __LIBC_DOUBLE_SIGN;
	}
	if (quotient != nullptr)
		*quotient = negative_quotient ? -(int)(q & 0x7fffffff) : (int)(q & 0x7fffffff);
	if (mx == 0)
		return __libc_double_from(bx & __LIBC_DOUBLE_SIGN);

	/* r = mx * 2^(ey - 1075), exactly representable: normalise, or shift into the subnormal range */
	for (; mx < (1ULL << 52); --ey)
		mx <<= 1;
	if (ey <= 0) {
		mx >>= 1 - ey;
		ey = 0;
	}

	return __libc_double_from(sign | ((uint64_t)ey << 52) | (mx & __LIBC_DOUBLE_FRACTION));
}
