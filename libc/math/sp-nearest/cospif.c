/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cospif.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.4.11: cos(pi x) = sin(pi x + pi/2) with an exact reduction in
 * float and the sin core; cos(pi (n + 1/2)) is +0 and cos(pi n) is +-1
 * exactly (IEEE 754 9.2.1) (PLAN D60).
 */
float cospif(float x)
{
	float y;
	float n;
	float d;
	int k;
	struct __libc_ff value;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return __libc_math_invalidf();
	/* Every float from 2^24 up is even, and from 2^23 an integer */
	if (fabsf(x) >= 0x1p24f)
		return 1.0f;
	if (fabsf(x) >= 0x1p23f)
		return ((int32_t)x & 1) == 0 ? 1.0f : -1.0f;

	/* 32 x = K + d exactly (|x| below 2^23, |d| <= 1/2); the angle is K pi/32 + d pi/32 */
	y = 32.0f * x;
	n = roundf(y);
	k = (int)n;
	d = y - n;
	if (d == 0 && (k & 15) == 0)
		return (k & 16) != 0 ? 0.0f : (k & 63) == 0 ? 1.0f :
													  -1.0f;

	value = __libc_sinf_core(k + 16, __libc_ff_scale(d, __LIBC_PI_32F));

	return value.hi + value.lo;
}
