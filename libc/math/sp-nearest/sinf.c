/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sinf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.4.6: x reduced exactly to k pi/32 + s, then the sin core (PLAN D60) */
float sinf(float x)
{
	struct __libc_ff s;
	struct __libc_ff value;
	int k;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return __libc_math_invalidf();
	/* The x^3 term is below half an ulp of x: the result rounds to x (keeping -0) */
	if (fabsf(x) < 0x1p-12f)
		return x;

	if (fabsf(x) < 0x1p-6f) {
		k = 0;
		s = (struct __libc_ff){ .hi = x, .lo = 0.0f };
	} else {
		k = __libc_rem_pio2f(x, &s);
	}
	value = __libc_sinf_core(k, s);

	return value.hi + value.lo;
}
