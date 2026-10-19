/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log1p_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* log(1 + x) = log(s) + log1p(t/s) with 1 + x = s + t exactly; t/s is below 2^-53, so its log1p is t/s;
 * rounding to nearest, where TwoSum is exact */
struct __libc_dd __libc_log1p_core(double x)
{
	int mode = __libc_math_nearest();
	struct __libc_dd s = __libc_dd_sum(1.0, x);
	struct __libc_dd log = __libc_log_core(s.hi);
	struct __libc_dd result = __libc_dd_fast_sum(log.hi, log.lo + s.lo / s.hi);

	__libc_math_restore(mode);
	return result;
}
