/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log2.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.16, F.10.3: log(+-0) is -infinity (divide-by-zero), a negative argument is a domain error */
double log2(double x)
{
	struct __libc_dd log;

	if (isnan(x))
		return x + x;
	if (x == 0)
		return __libc_math_divbyzero(-1.0);
	if (x < 0)
		return __libc_math_invalid();
	if (isinf(x) || x == 1.0)
		return isinf(x) ? x : 0.0;
	/* Powers of two exactly, subnormal ones included */
	if (scalbn(x, -ilogb(x)) == 1.0)
		return ilogb(x);

	log = __libc_log_core(x);
	log = __libc_dd_multiply(log, __LIBC_INV_LN2);
	return log.hi + log.lo;
}
