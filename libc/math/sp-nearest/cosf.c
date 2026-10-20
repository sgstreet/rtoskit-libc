/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cosf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.5: cos x = sin(x + pi/2), so the core with k + 16 (PLAN D60) */
float cosf(float x)
{
	struct __libc_ff s;
	struct __libc_ff value;
	int k;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return __libc_math_invalidf();

	if (fabsf(x) < 0x1p-6f) {
		k = 0;
		s = (struct __libc_ff){ .hi = x, .lo = 0.0f };
	} else {
		k = __libc_rem_pio2f(x, &s);
	}
	value = __libc_sinf_core(k + 16, s);

	return value.hi + value.lo;
}
