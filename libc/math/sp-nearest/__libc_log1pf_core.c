/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log1pf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* log(1 + t) as float-float for a float-float t above -1: the series below 2^-6, else the log2 core on 1 + t (PLAN D58, D67) */
struct __libc_ff __libc_log1pf_core(struct __libc_ff t)
{
	struct __libc_ff sum;

	if (fabsf(t.hi) < 0x1p-6f)
		return __libc_ff_log1p_series(t);

	sum = __libc_ff_add((struct __libc_ff){ .hi = 1.0f, .lo = 0.0f }, t);

	return __libc_ff_multiply(__libc_log2f_core(sum.hi, sum.lo), __LIBC_LN2F);
}
