/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log2p1f_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * log2(1 + x) for finite x > -1 above 2^-90 in magnitude: near 0 the series
 * on x itself keeps the relative accuracy; elsewhere 1 + x is formed
 * exactly as float-float and goes through the log2 core (PLAN D58).
 */
struct __libc_ff __libc_log2p1f_core(float x)
{
	struct __libc_ff sum;

	if (fabsf(x) < 0x1p-6f)
		return __libc_ff_multiply(__libc_ff_log1p_series((struct __libc_ff){ .hi = x, .lo = 0.0f }), __LIBC_INV_LN2F);

	sum = __libc_ff_sum(1.0f, x);

	return __libc_log2f_core(sum.hi, sum.lo);
}
