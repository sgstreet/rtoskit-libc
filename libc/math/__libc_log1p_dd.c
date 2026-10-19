/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log1p_dd.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* log(1 + u): 1 + u = s + t exactly, then log(s) + t/s (t/s is below 2^-52); rounding to nearest is expected */
struct __libc_dd __libc_log1p_dd(struct __libc_dd u)
{
	struct __libc_dd s = __libc_dd_sum(1.0, u.hi);
	struct __libc_dd log;

	s = __libc_dd_fast_sum(s.hi, s.lo + u.lo);
	log = __libc_log_core(s.hi);

	return __libc_dd_fast_sum(log.hi, log.lo + s.lo / s.hi);
}
