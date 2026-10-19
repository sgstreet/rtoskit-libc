/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * acosh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.5.1, F.10.2.1: log1p(t + sqrt(2t + t^2)) with t = x - 1, log(2x) beyond 2^28; x < 1 is a domain error */
double acosh(double x)
{
	struct __libc_dd value;
	struct __libc_dd t;
	int mode;

	if (isnan(x))
		return x + x;
	if (x < 1.0)
		return __libc_math_invalid();
	if (x == 1.0)
		return 0.0;
	if (isinf(x))
		return x;

	mode = __libc_math_nearest();
	if (x > 0x1p28) {
		value = __libc_dd_add(__libc_log_core(x), __LIBC_LN2);
	} else {
		t = __libc_dd_sum(x, -1.0);
		value = __libc_dd_sqrt(__libc_dd_multiply(t, __libc_dd_add(t, (struct __libc_dd){ .hi = 2.0, .lo = 0.0 })));
		value = __libc_log1p_dd(__libc_dd_add(t, value));
	}
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
