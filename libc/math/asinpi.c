/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * asinpi.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.9: the angle divided by pi as a double-double (docs/research/libm-trig.md) */
double asinpi(double x)
{
	struct __libc_dd root;
	struct __libc_dd value;
	int mode;

	if (isnan(x) || x == 0)
		return x + x;
	if (isgreater(fabs(x), 1.0))
		return __libc_math_invalid();
	if (fabs(x) == 1.0)
		return copysign(0.5, x);
	if (fabs(x) < 0x1p-27)
		return __libc_dd_scale_product(x, __LIBC_INV_PI);

	mode = __libc_math_nearest();
	root = __libc_dd_sqrt(__libc_dd_multiply(__libc_dd_sum(1.0, -x), __libc_dd_sum(1.0, x)));
	value = __libc_atan2_core((struct __libc_dd){ .hi = x, .lo = 0.0 }, root);
	value = __libc_dd_multiply(value, __LIBC_INV_PI);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
