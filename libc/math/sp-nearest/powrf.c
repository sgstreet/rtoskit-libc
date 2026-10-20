/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * powrf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.7.7: e^(y log x) for x >= 0 only, with powr's special cases
 * (IEEE 754 9.2.1: 0^0, inf^0 and 1^inf are invalid; powr(0, -inf) is +inf
 * without divide-by-zero), then as powf (PLAN D59).
 */
float powrf(float x, float y)
{
	struct __libc_ff log;
	struct __libc_ff z;

	if (isnan(x) || isnan(y))
		return x + y;
	if (x < 0)
		return __libc_math_invalidf();
	if (x == 0) {
		if (y == 0)
			return __libc_math_invalidf();
		if (y < 0)
			return isinf(y) ? INFINITY : 1.0f / fabsf(x);
		return 0.0f;
	}
	if (isinf(x)) {
		if (y == 0)
			return __libc_math_invalidf();
		return y < 0 ? 0.0f : INFINITY;
	}
	if (x == 1.0f)
		return isinf(y) ? __libc_math_invalidf() : 1.0f;
	if (isinf(y))
		return (x < 1.0f) == (y < 0) ? INFINITY : 0.0f;
	if (y == 0)
		return 1.0f;

	log = __libc_log2f_core(x, 0.0f);
	if (fabsf(y) > 0x1p31f)
		z = (struct __libc_ff){ .hi = y * log.hi, .lo = 0.0f };
	else
		z = __libc_ff_multiply(log, (struct __libc_ff){ .hi = y, .lo = 0.0f });

	return __libc_exp2f_signed(z, false);
}
