/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tan.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.7, F.10.1: an infinity is a domain error; the reduction is exact for every finite x (D48) */
double tan(double x)
{
	struct __libc_dd r;
	struct __libc_dd value;
	int quadrant;
	int mode;

	if (isnan(x))
		return x + x;
	if (isinf(x))
		return __libc_math_invalid();
	if (fabs(x) < 0x1p-27)
		return __libc_math_tiny(x, x);

	mode = __libc_math_nearest();
	quadrant = __libc_rem_pio2(x, &r);
	if ((quadrant & 1) == 0)
		value = __libc_dd_divide(__libc_sin_core(r), __libc_cos_core(r));
	else
		value = __libc_dd_negate(__libc_dd_divide(__libc_cos_core(r), __libc_sin_core(r)));
	__libc_math_restore(mode);
	return value.hi + value.lo;
}
