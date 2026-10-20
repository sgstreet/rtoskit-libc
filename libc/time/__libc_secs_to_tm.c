/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_secs_to_tm.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <time.h>

#include "internal/time.h"

bool __libc_secs_to_tm(long long t, struct tm *tm)
{
	/* Floor division, so times before the epoch land on the right day */
	uint32_t seconds;
	long long days = __libc_floor_divmod64(t, __LIBC_SECONDS_PER_DAY, &seconds);
	uint32_t weekday;
	long long year;
	unsigned int month;
	unsigned int day;

	__libc_civil_from_days(days, &year, &month, &day);
	if (year - 1900 > INT_MAX || year - 1900 < INT_MIN) {
		errno = EOVERFLOW;
		return false;
	}

	tm->tm_year = (int)(year - 1900);
	tm->tm_mon = (int)month - 1;
	tm->tm_mday = (int)day;
	tm->tm_yday = (int)(days - __libc_days_from_civil(year, 1, 1));
	/* 1970-01-01 was a Thursday */
	__libc_floor_divmod64(days + 4, 7, &weekday);
	tm->tm_wday = (int)weekday;
	tm->tm_hour = (int)(seconds / 3600);
	tm->tm_min = (int)(seconds / 60 % 60);
	tm->tm_sec = (int)(seconds % 60);

	return true;
}
