/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * mktime.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/*
 * C23 7.29.2.3: the fields as local time, normalised. tm_isdst positive or
 * zero says which offset applies; negative asks for the one in force: standard
 * time unless that instant is in daylight saving time and the daylight saving
 * reading is too. In a spring gap neither is, and standard time is used; in an
 * autumn overlap both are, and standard (the later) time is used.
 */
time_t mktime(struct tm *timeptr)
{
	struct __libc_tz tz;
	long long local = __libc_tm_to_secs(timeptr);
	time_t t;

	__libc_tz_get(&tz);
	t = local - tz.std_offset;
	if (tz.has_dst) {
		time_t daylight = local - tz.dst_offset;

		if (timeptr->tm_isdst > 0 || (timeptr->tm_isdst < 0 && __libc_tz_is_dst(&tz, t) && __libc_tz_is_dst(&tz, daylight)))
			t = daylight;
	}

	if (localtime_r(&t, timeptr) == nullptr)
		return (time_t)-1;

	return t;
}
