/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atanhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.5.3: below 1/8 the odd series x + x^3/3 + x^5/5 + ... + x^13/13
 * (cube and 1/3 as float-float); above, log1p(2|x|/(1 - |x|))/2 with the
 * quotient as float-float through the log2 core; +-1 gives +-infinity with
 * divide-by-zero (PLAN D62).
 */
float atanhf(float x)
{
	float a = fabsf(x);
	struct __libc_ff value;
	struct __libc_ff square;

	if (isnan(x))
		return x + x;
	if (a > 1.0f)
		return __libc_math_invalidf();
	if (a == 1.0f)
		return x / 0.0f;
	if (a < 0x1p-12f)
		return x;

	if (a < 0.125f) {
		square = __libc_ff_product(x, x);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = x, .lo = 0.0f }, square), __libc_ff_fast_sum(0x1.555556p-2f, -0x1.555556p-27f + square.hi * (0x1.99999ap-3f + square.hi * (0x1.24924ap-3f + square.hi * (0x1.c71c72p-4f + square.hi * (0x1.745d18p-4f + square.hi * 0x1.3b13b2p-4f))))));
		value = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, value);
		return value.hi + value.lo;
	}

	value = __libc_ff_divide((struct __libc_ff){ .hi = 2.0f * a, .lo = 0.0f }, __libc_ff_sum(1.0f, -a));
	value = __libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, value);
	value = __libc_ff_multiply(__libc_log2f_core(value.hi, value.lo), __LIBC_LN2F);

	return copysignf(0.5f * (value.hi + value.lo), x);
}
