/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * compoundnf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.7.2: (1 + x)^n = 2^(n log2(1 + x)), both as float-float (PLAN D59) */
float compoundnf(float x, long long n)
{
	if (isnan(x))
		return n == 0 ? 1.0f : x + x;
	if (x < -1.0f)
		return __libc_math_invalidf();
	if (n == 0)
		return 1.0f;
	if (x == -1.0f)
		return n < 0 ? 1.0f / fabsf(x + 1.0f) : 0.0f;
	if (isinf(x))
		return n < 0 ? 0.0f : INFINITY;
	if (x == 0)
		return 1.0f;
	if (n == 1)
		return 1.0f + x;
	/* 1 + n x + ... with |n x| below 2^-27: rounds to 1 */
	if (fabsf(x) < 0x1p-90f)
		return 1.0f;

	return __libc_exp2f_signed(__libc_ff_multiply(__libc_log2p1f_core(x), __libc_ff_from_i64(n)), false);
}
