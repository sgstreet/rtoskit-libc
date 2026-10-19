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

/* C23 7.29.2.2: exact while the difference fits, rounded only once by the conversion */
double difftime(time_t time1, time_t time0)
{
	time_t difference;

	if (!ckd_sub(&difference, time1, time0))
		return (double)difference;

	return (double)time1 - (double)time0;
}
