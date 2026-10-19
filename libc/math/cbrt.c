/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cbrt.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.7.1: x = y * 2^(3q) with y in [1, 8); Newton steps from a linear
 * guess give cbrt(y) to 2^-30, rounded to 20 bits so t^2 is exact, then one
 * Halley step t * (t^3 + 2y) / (2t^3 + y), written as t + t(s - t)/(2t + s)
 * with s = y/t^2, leaves about 0.6 ulp.
 */
double cbrt(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits);
	int scale = 0;
	int q;
	double y;
	double t;
	double s;

	if (exponent == 0x7ff || x == 0)
		return x + x;
	if (exponent == 0) {
		bits = __libc_double_bits(x * 0x1p54);
		exponent = __libc_double_exponent(bits);
		scale = -18;
	}

	exponent -= __LIBC_DOUBLE_BIAS;
	q = exponent >= 0 ? exponent / 3 : -((-exponent + 2) / 3);
	y = __libc_double_from((bits & __LIBC_DOUBLE_FRACTION) | ((uint64_t)(__LIBC_DOUBLE_BIAS + exponent - 3 * q) << 52));

	t = 1.0 + (y - 1.0) / 7.0;
	for (int i = 0; i < 5; ++i)
		t -= (t * t * t - y) / (3.0 * t * t);
	t = __libc_double_from(__libc_double_bits(t) & ~((1ULL << 33) - 1));
	s = y / (t * t);
	t += t * (s - t) / (t + t + s);

	return copysign(scalbn(t, q + scale), x);
}
