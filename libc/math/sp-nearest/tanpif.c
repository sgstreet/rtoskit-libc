/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tanpif.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.4.15: tan(pi x) as the quotient of the sin core at the angle and
 * a quarter turn on, after an exact reduction in float; the zeros and the
 * poles of IEEE 754 9.2.1 directly (tan(pi n) is +-0 with the sign of x for
 * an even n and of -x for an odd one, tan(pi (n + 1/2)) is +-infinity with
 * divide-by-zero); far below 1, pi x rounded once (PLAN D60).
 */
float tanpif(float x)
{
	float y;
	float n;
	float d;
	int k;
	int shift;
	struct __libc_ff s;
	struct __libc_ff value;

	if (isnan(x) || x == 0)
		return x + x;
	if (isinf(x))
		return __libc_math_invalidf();
	if (fabsf(x) >= 0x1p24f)
		return copysignf(0.0f, x);
	if (fabsf(x) >= 0x1p23f)
		return ((int32_t)x & 1) == 0 ? copysignf(0.0f, x) : copysignf(0.0f, -x);
	if (fabsf(x) < 0x1p-90f) {
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_PIF), shift);
	}

	/* 32 x = K + d exactly (|x| below 2^23, |d| <= 1/2); the angle is K pi/32 + d pi/32 */
	y = 32.0f * x;
	n = roundf(y);
	k = (int)n;
	d = y - n;
	if (d == 0 && (k & 15) == 0) {
		switch ((k & 63) / 16) {
			case 0:
				return copysignf(0.0f, x);
			case 2:
				return copysignf(0.0f, -x);
			case 1:
				return 1.0f / fabsf(d);
			default:
				return -1.0f / fabsf(d);
		}
	}

	s = __libc_ff_scale(d, __LIBC_PI_32F);
	value = __libc_ff_divide(__libc_sinf_core(k, s), __libc_sinf_core(k + 16, s));

	return value.hi + value.lo;
}
