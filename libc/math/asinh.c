/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * asinh.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.5.2, F.10.2.2: log1p(|x| + x^2/(1 + sqrt(1 + x^2))), log(2|x|) beyond 2^28 */
double asinh(double x)
{
	struct __libc_dd value;
	struct __libc_dd square;
	struct __libc_dd root;
	int mode;
	double a = fabs(x);

	if (isnan(x) || isinf(x) || x == 0)
		return x + x;
	if (a < 0x1p-27)
		return __libc_math_tiny(x, -x);

	mode = __libc_math_nearest();
	if (a > 0x1p28) {
		value = __libc_dd_add(__libc_log_core(a), __LIBC_LN2);
	} else {
		square = __libc_dd_product(a, a);
		root = __libc_dd_sqrt(__libc_dd_add(square, (struct __libc_dd){ .hi = 1.0, .lo = 0.0 }));
		value = __libc_dd_divide(square, __libc_dd_add(root, (struct __libc_dd){ .hi = 1.0, .lo = 0.0 }));
		value = __libc_log1p_dd(__libc_dd_add((struct __libc_dd){ .hi = a, .lo = 0.0 }, value));
	}
	if (signbit(x))
		value = __libc_dd_negate(value);
	__libc_math_restore(mode);

	return value.hi + value.lo;
}
