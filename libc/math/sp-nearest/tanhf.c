/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tanhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.5.6: below 1/8 the odd series x - x^3/3 + 2x^5/15 - 17x^7/315 +
 * 62x^9/2835 - 1382x^11/155925 (cube and 1/3 as float-float); above, (1 - t)/(1 + t) with
 * t = e^-2|x| from the exp2 core as float-float; +-1 beyond 10, where
 * 2 e^-2|x| is below half an ulp of 1 (PLAN D62).
 */
float tanhf(float x)
{
	struct __libc_ff value;
	struct __libc_ff square;
	struct __libc_ff t;
	int exponent;

	if (isnan(x))
		return x + x;
	if (fabsf(x) < 0x1p-12f)
		return x;
	if (fabsf(x) > 10.0f)
		return copysignf(1.0f, x);

	if (fabsf(x) < 0.125f) {
		square = __libc_ff_product(x, x);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = x, .lo = 0.0f }, square), __libc_ff_fast_sum(-0x1.555556p-2f, 0x1.555556p-27f + square.hi * (0x1.111112p-3f - square.hi * (0x1.ba1ba2p-5f - square.hi * (0x1.664f48p-6f - square.hi * 0x1.226e36p-7f)))));
		value = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, value);
		return value.hi + value.lo;
	}

	t = __libc_exp2f_core(__libc_ff_scale(-2.0f * fabsf(x), __LIBC_INV_LN2F), &exponent);
	t = (struct __libc_ff){ .hi = scalbnf(t.hi, exponent), .lo = scalbnf(t.lo, exponent) };
	value = __libc_ff_divide(__libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, (struct __libc_ff){ .hi = -t.hi, .lo = -t.lo }), __libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, t));

	return copysignf(value.hi + value.lo, x);
}
