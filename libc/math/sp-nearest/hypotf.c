/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hypotf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.7.4, F.10.4.4: sqrt(a^2 + b^2) with the squares and their sum
 * as float-float and one correction of sqrtf by the exact residual; the
 * operands are scaled so that the squares stay normal, and the result is
 * scaled back with one rounding, subnormal results included (PLAN D59).
 */
float hypotf(float x, float y)
{
	float a = fabsf(x);
	float b = fabsf(y);
	int scale = 0;
	float h;
	struct __libc_ff a2;
	struct __libc_ff b2;
	struct __libc_ff sum;
	struct __libc_ff h2;

	/* An infinity wins even over a NaN */
	if (isinf(x) || isinf(y))
		return INFINITY;
	if (isnan(x) || isnan(y))
		return x + y;
	if (a < b) {
		float swap = a;

		a = b;
		b = swap;
	}
	/* b below 2^-30 a adds less than half an ulp */
	if (b == 0 || ilogbf(a) - ilogbf(b) > 30)
		return a + b;

	if (a > 0x1p60f) {
		a *= 0x1p-70f;
		b *= 0x1p-70f;
		scale = 70;
	} else if (b < 0x1p-60f) {
		a *= 0x1p90f;
		b *= 0x1p90f;
		scale = -90;
	}

	a2 = __libc_ff_product(a, a);
	b2 = __libc_ff_product(b, b);
	sum = __libc_ff_sum(a2.hi, b2.hi);
	sum.lo += a2.lo + b2.lo;
	h = sqrtf(sum.hi + sum.lo);
	h2 = __libc_ff_product(h, h);

	return __libc_exp2f_scale(__libc_ff_fast_sum(h, (((sum.hi - h2.hi) - h2.lo) + sum.lo) / (2.0f * h)), scale);
}
