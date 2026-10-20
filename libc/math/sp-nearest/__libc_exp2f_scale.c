/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2f_scale.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * The value rounded once: a normal result rounds v and scales it exactly. A
 * subnormal one is rounded on the 2^-149 grid directly: scaled by 2^126 it
 * lies below 1 in magnitude, where adding +-1 (the sign of v) puts the grid
 * at the ulp of [1, 2), so the sum rounds exactly as the subnormal result
 * must (PLAN D57).
 */
float __libc_exp2f_scale(struct __libc_ff v, int exponent)
{
	float high;
	float bias;
	struct __libc_ff sum;

	if (exponent > -126)
		return scalbnf(v.hi + v.lo, exponent);

	high = scalbnf(v.hi, exponent + 126);
	if (fabsf(high) >= 1.0f)
		return scalbnf(v.hi + v.lo, exponent);

	bias = copysignf(1.0f, v.hi);
	sum = __libc_ff_sum(bias, high);

	return ((sum.hi + (sum.lo + scalbnf(v.lo, exponent + 126))) - bias) * 0x1p-126f;
}
