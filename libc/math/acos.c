/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * acos.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.1, F.10.1.1: atan2(sqrt((1 - x)(1 + x)), x); acos(1) is +0, |x| > 1 a domain error */
double acos(double x)
{
	struct __libc_dd root;
	struct __libc_dd value;
	int mode;

	if (isnan(x))
		return x + x;
	if (isgreater(fabs(x), 1.0))
		return __libc_math_invalid();
	if (x == 1.0)
		return 0.0;

	mode = __libc_math_nearest();
	root = __libc_dd_sqrt(__libc_dd_multiply(__libc_dd_sum(1.0, -x), __libc_dd_sum(1.0, x)));
	value = __libc_atan2_core(root, (struct __libc_dd){ .hi = x, .lo = 0.0 });
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
