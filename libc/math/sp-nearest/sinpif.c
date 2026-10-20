/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sinpif.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.4.13: sin(pi x) with an exact reduction in float, the sin core,
 * and the exact zeros and ones of IEEE 754 9.2.1 returned directly; far
 * below 1 the result is pi x rounded once (PLAN D60).
 */
float sinpif(float x)
{
	float y;
	float n;
	float d;
	int k;
	int shift;
	struct __libc_ff value;

	if (isnan(x) || x == 0)
		return x + x;
	if (isinf(x))
		return __libc_math_invalidf();
	/* Every float from 2^23 up is an integer: sin(pi n) is +-0 with the sign of x */
	if (fabsf(x) >= 0x1p23f)
		return copysignf(0.0f, x);
	if (fabsf(x) < 0x1p-90f) {
		shift = ilogbf(x);
		return __libc_exp2f_scale(__libc_ff_scale(scalbnf(x, -shift), __LIBC_PIF), shift);
	}

	/* 32 x = K + d exactly (|x| below 2^23, |d| <= 1/2); the angle is K pi/32 + d pi/32 */
	y = 32.0f * x;
	n = roundf(y);
	k = (int)n;
	d = y - n;
	/* x a multiple of 1/2: an integer gives +-0 with the sign of x, n + 1/2 gives +-1 */
	if (d == 0 && (k & 15) == 0)
		return (k & 16) == 0 ? copysignf(0.0f, x) : (k & 63) == 16 ? 1.0f :
																	 -1.0f;

	value = __libc_sinf_core(k, __libc_ff_scale(d, __LIBC_PI_32F));

	return value.hi + value.lo;
}
