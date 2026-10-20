/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_expf_pair.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * e^|x| = 2^k (v.hi + v.lo) through the exp2 core, and *inverse the
 * reciprocal 2^-k / v scaled into the same binade as v (by 2^-2k), so that
 * sinh and cosh are v -+ inverse times 2^(k - 1) with one rounding. The
 * reciprocal is 2^-2k of v: for k above 20 it is below 2^-40 of the result
 * and left out (PLAN D62).
 */
struct __libc_ff __libc_expf_pair(float x, int *exponent, struct __libc_ff *inverse)
{
	struct __libc_ff value = __libc_exp2f_core(__libc_ff_scale(fabsf(x), __LIBC_INV_LN2F), exponent);
	struct __libc_ff reciprocal;

	if (*exponent > 20) {
		*inverse = (struct __libc_ff){ .hi = 0.0f, .lo = 0.0f };
	} else {
		reciprocal = __libc_ff_divide((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, value);
		*inverse = (struct __libc_ff){ .hi = scalbnf(reciprocal.hi, -2 * *exponent), .lo = scalbnf(reciprocal.lo, -2 * *exponent) };
	}

	return value;
}
