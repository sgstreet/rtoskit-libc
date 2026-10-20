/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sinhcoshf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * sinh x and cosh x as float-float for |x| below 16: below 2^-20 x itself
 * and 1 + x^2/2 (the next terms are below 2^-42; x keeps its sign and stays
 * exact when subnormal), below 1/4 the odd series of sinh (cube and 1/6
 * exact, as sinhf) and cosh = sqrt(1 + sinh^2);
 * above, (e^|x| -+ e^-|x|)/2 from the exp pair, the sign of x restored
 * (PLAN D62, D65).
 */
void __libc_sinhcoshf_core(float x, struct __libc_ff *sinh, struct __libc_ff *cosh)
{
	struct __libc_ff value;
	struct __libc_ff inverse;
	struct __libc_ff square;
	int exponent;

	if (fabsf(x) < 0x1p-20f) {
		*sinh = (struct __libc_ff){ .hi = x, .lo = 0.0f };
		*cosh = (struct __libc_ff){ .hi = 1.0f, .lo = 0.5f * x * x };
		return;
	}
	if (fabsf(x) < 0.25f) {
		square = __libc_ff_product(x, x);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = x, .lo = 0.0f }, square), __libc_ff_fast_sum(0x1.555556p-3f, -0x1.555556p-28f + square.hi * (0x1.111112p-7f + square.hi * (0x1.a01a02p-13f + square.hi * 0x1.71de3ap-19f))));
		*sinh = __libc_ff_add((struct __libc_ff){ .hi = x, .lo = 0.0f }, value);
		*cosh = __libc_ff_sqrt(__libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, __libc_ff_multiply(*sinh, *sinh)));
		return;
	}

	value = __libc_expf_pair(x, &exponent, &inverse);
	*sinh = __libc_ff_add(value, (struct __libc_ff){ .hi = -inverse.hi, .lo = -inverse.lo });
	*sinh = (struct __libc_ff){ .hi = copysignf(scalbnf(sinh->hi, exponent - 1), x), .lo = copysignf(1.0f, x) * scalbnf(sinh->lo, exponent - 1) };
	*cosh = __libc_ff_add(value, inverse);
	*cosh = (struct __libc_ff){ .hi = scalbnf(cosh->hi, exponent - 1), .lo = scalbnf(cosh->lo, exponent - 1) };
}
