/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp_scale.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * 2^exponent * (hi + lo) with one rounding. A normal result rounds the
 * double-double, then scales exactly (or overflows, which scalbn raises).
 * A subnormal result is scaled into [0, 1) by 2^(exponent + 1022), which is
 * exact for hi, and added to 1 (or -1): the sum's ulp, 2^-52, is the
 * subnormal ulp scaled the same way, so one addition rounds in the current
 * direction, and the bias comes off exactly (a zero keeps the sign). Underflow is raised when the
 * rounding was inexact (docs/research/libm-exp-log.md).
 */
double __libc_exp_scale(struct __libc_dd value, int exponent)
{
	double bias;
	double hi;
	double lo;
	double tail;
	double rounded;
	struct __libc_dd sum;
	int mode;

	if (value.hi == 0 || ilogb(value.hi) + exponent >= -1022)
		return scalbn(value.hi + value.lo, exponent);
	if (ilogb(value.hi) + exponent < -1076)
		return __libc_math_underflow(copysign(1.0, value.hi));

	hi = scalbn(value.hi, exponent + 1022);
	lo = scalbn(value.lo, exponent + 1022);
	bias = copysign(1.0, hi);
	mode = __libc_math_nearest();
	sum = __libc_dd_sum(bias, hi);
	tail = sum.lo + lo;
	__libc_math_restore(mode);
	rounded = sum.hi + tail;
	if (rounded - sum.hi != tail)
		__libc_math_raise_underflow();

	return copysign(rounded - bias, bias) * 0x1p-1022;
}
