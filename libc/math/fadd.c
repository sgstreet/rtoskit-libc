/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fadd.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.14.1: x + y rounded to float once (not to double first). The
 * exact sum is x * 1 + y; zeros, infinities and NaNs take the ordinary
 * addition, exact cancellation too (it gives the zero the direction
 * prescribes).
 */
float fadd(double x, double y)
{
	struct __libc_exact sum;
	int ex;
	int ey;
	uint64_t mx;
	uint64_t my;

	if (!isfinite(x) || !isfinite(y) || x == 0 || y == 0)
		return (float)(x + y);

	mx = __libc_double_split(x, &ex);
	my = __libc_double_split(y, &ey);
	if (!__libc_exact_fma(&sum, signbit(x), mx, 1, ex, signbit(y), my, ey))
		return (float)(x + y);

	return __libc_exact_to_float(&sum);
}
