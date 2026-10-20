/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rsqrtf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.7.9: r = 1/sqrtf(x) is within about an ulp; one Newton step
 * r + r (1 - x r^2)/2 with the residual from exact products (Dekker) leaves
 * an error near 2^-46, so the last rounding is almost always correct.
 * x is scaled into [2^-100, 2^100] first, keeping r^2 normal (PLAN D59).
 */
float rsqrtf(float x)
{
	int scale = 0;
	float root;
	float residual;
	struct __libc_ff square;
	struct __libc_ff product;

	if (isnan(x))
		return x + x;
	/* +-infinity with divide-by-zero */
	if (x == 0)
		return (signbit(x) ? -1.0f : 1.0f) / fabsf(x);
	if (x < 0)
		return __libc_math_invalidf();
	if (isinf(x))
		return 0.0f;

	if (x < 0x1p-100f) {
		x *= 0x1p100f;
		scale = 50;
	} else if (x > 0x1p100f) {
		x *= 0x1p-100f;
		scale = -50;
	}

	root = 1.0f / sqrtf(x);
	square = __libc_ff_product(root, root);
	product = __libc_ff_product(x, square.hi);
	residual = ((1.0f - product.hi) - product.lo) - x * square.lo;

	return scalbnf(root + root * (0.5f * residual), scale);
}
