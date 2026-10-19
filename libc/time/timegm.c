/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * timegm.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/* C23 7.29.2.5: mktime for UTC; the fields come back normalised */
time_t timegm(struct tm *timeptr)
{
	time_t t = __libc_tm_to_secs(timeptr);

	if (gmtime_r(&t, timeptr) == nullptr)
		return (time_t)-1;

	return t;
}
