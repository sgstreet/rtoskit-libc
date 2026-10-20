/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * difftime.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdckdint.h>
#include <time.h>

#include "internal/math.h"

/* C23 7.29.2.2: exact while the difference fits, rounded only once by the conversion (without the soft-float helpers, PLAN D55) */
double difftime(time_t time1, time_t time0)
{
	time_t difference;

	if (!ckd_sub(&difference, time1, time0))
		return __libc_i64_to_double(difference);

	return __libc_i64_to_double(time1) - __libc_i64_to_double(time0);
}
