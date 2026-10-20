/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cbrtf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.7.1: |x| = m 2^(3q) with m in [1, 8); four Newton steps in float
 * from 1 + (m - 1)/7 reach the float limit, then one step with the residual
 * m - t^3 from exact products (Dekker) leaves an error near 2^-46; the
 * scaling back is exact (PLAN D59).
 */
float cbrtf(float x)
{
	float a = fabsf(x);
	int scale = 0;
	int exponent;
	int q;
	float m;
	float t;
	struct __libc_ff square;
	struct __libc_ff cube;

	if (isnan(x) || isinf(x) || x == 0)
		return x + x;
	if (a < 0x1p-126f) {
		a *= 0x1p24f;
		scale = -8;
	}

	exponent = ilogbf(a);
	q = exponent >= 0 ? exponent / 3 : -((-exponent + 2) / 3);
	m = scalbnf(a, -3 * q);

	t = 1.0f + (m - 1.0f) * 0x1.24924ap-3f;
	for (int i = 0; i < 4; ++i)
		t -= (t * t * t - m) / (3.0f * t * t);

	square = __libc_ff_product(t, t);
	cube = __libc_ff_multiply(square, (struct __libc_ff){ .hi = t, .lo = 0.0f });
	t += ((m - cube.hi) - cube.lo) / (3.0f * square.hi);

	return copysignf(scalbnf(t, q + scale), x);
}
