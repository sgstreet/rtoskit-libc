/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tm_to_secs.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/*
 * C23 7.29.2.3p2: the fields may lie outside their ranges. Months fold into
 * years first, then everything else is plain arithmetic on days and seconds;
 * with int fields the result always fits 64 bits.
 */
long long __libc_tm_to_secs(const struct tm *tm)
{
	long long year = (long long)tm->tm_year + 1900;
	long long month = tm->tm_mon;

	year += month / 12;
	month %= 12;
	if (month < 0) {
		month += 12;
		--year;
	}

	long long days = __libc_days_from_civil(year, (unsigned int)month + 1, 1) + tm->tm_mday - 1;

	return days * __LIBC_SECONDS_PER_DAY + (long long)tm->tm_hour * 3600 + (long long)tm->tm_min * 60 + tm->tm_sec;
}
