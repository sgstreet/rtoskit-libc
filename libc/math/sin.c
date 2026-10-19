/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sin.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.4.6, F.10.1: an infinity is a domain error; the reduction is exact for every finite x (D48) */
double sin(double x)
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
		return __libc_math_tiny(x, -x);

	mode = __libc_math_nearest();
	quadrant = __libc_rem_pio2(x, &r);
	switch (quadrant) {
		case 0:
			value = __libc_sin_core(r);
			break;
		case 1:
			value = __libc_cos_core(r);
			break;
		case 2:
			value = __libc_dd_negate(__libc_sin_core(r));
			break;
		default:
			value = __libc_dd_negate(__libc_cos_core(r));
			break;
	}
	__libc_math_restore(mode);
	return value.hi + value.lo;
}
