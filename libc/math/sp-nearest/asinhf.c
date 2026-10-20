/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * asinhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.5.2: below 1/8 the odd series x - x^3/6 + 3x^5/40 - 15x^7/336 +
 * 105x^9/3456 - 945x^11/42240 (cube and 1/6 as float-float); up to 2^20, log(|x| + sqrt(x^2 + 1)) with
 * the argument as float-float through the log2 core; beyond, log(2|x|)
 * (PLAN D62).
 */
float asinhf(float x)
{
	float a = fabsf(x);
	struct __libc_ff value;
	struct __libc_ff square;

	if (isnan(x) || isinf(x))
		return x + x;
	if (a < 0x1p-12f)
		return x;

	if (a < 0.125f) {
		square = __libc_ff_product(x, x);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = x, .lo = 0.0f }, square), __libc_ff_fast_sum(-0x1.555556p-3f, 0x1.555556p-28f + square.hi * (0x1.333334p-4f - square.hi * (0x1.6db6dcp-5f - square.hi * (0x1.f1c71cp-6f - square.hi * 0x1.6e8ba2p-6f)))));
		value = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, value);
		return value.hi + value.lo;
	}

	if (a > 0x1p20f) {
		value = __libc_log2f_core(a, 0.0f);
		value = __libc_ff_add(value, (struct __libc_ff){ .hi = 1.0f, .lo = 0.0f });
	} else {
		square = __libc_ff_product(a, a);
		square = __libc_ff_add(square, (struct __libc_ff){ .hi = 1.0f, .lo = 0.0f });
		value = __libc_ff_add((struct __libc_ff){ .hi = a, .lo = 0.0f }, __libc_ff_sqrt(square));
		value = __libc_log2f_core(value.hi, value.lo);
	}
	value = __libc_ff_multiply(value, __LIBC_LN2F);

	return copysignf(value.hi + value.lo, x);
}
