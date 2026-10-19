/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log10.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.12, F.10.3: log(+-0) is -infinity (divide-by-zero), a negative argument is a domain error */
double log10(double x)
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

	log = __libc_log_core(x);
	log = __libc_dd_multiply(log, __LIBC_INV_LN10);
	return log.hi + log.lo;
}
