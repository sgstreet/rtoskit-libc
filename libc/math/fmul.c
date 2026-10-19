/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fmul.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.14.3: x * y rounded to float once; the exact product of two doubles has 106 bits */
float fmul(double x, double y)
{
	struct __libc_exact product;
	int ex;
	int ey;
	uint64_t mx;
	uint64_t my;

	if (!isfinite(x) || !isfinite(y) || x == 0 || y == 0)
		return (float)(x * y);

	mx = __libc_double_split(x, &ex);
	my = __libc_double_split(y, &ey);
	__libc_exact_fma(&product, signbit(x) != signbit(y), mx, my, ex + ey, false, 0, 0);

	return __libc_exact_to_float(&product);
}
