/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tz_is_dst.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/* The day of the year (0-based) a rule picks in year */
static long long rule_day(const struct __libc_tz_rule *rule, long long year)
{
	bool leap = __libc_is_leap(year);

	switch (rule->kind) {
		case 'J':
			/* 1-365, February 29 never counted */
			return rule->day - 1 + (leap && rule->day >= 60);
		case 'M': {
			/* The day-th weekday of week week (5 means the last) of month */
			long long first = __libc_days_from_civil(year, rule->month, 1);
			long long start = first - __libc_days_from_civil(year, 1, 1);
			uint32_t first_wday;
			int day;

			__libc_floor_divmod64(first + 4, 7, &first_wday);
			day = (rule->day - (int)first_wday + 7) % 7 + (rule->week - 1) * 7;
			static const unsigned char lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			int length = lengths[rule->month - 1] + (rule->month == 2 && leap);

			while (day >= length)
				day -= 7;
			return start + day;
		}
		default:
			return rule->day;
	}
}

/* The UTC time of a change in year; offset is the one in force before it */
static long long change_time(const struct __libc_tz_rule *rule, long long year, long offset)
{
	return (__libc_days_from_civil(year, 1, 1) + rule_day(rule, year)) * __LIBC_SECONDS_PER_DAY + rule->time - offset;
}

bool __libc_tz_is_dst(const struct __libc_tz *tz, long long t)
{
	struct tm local;

	if (!tz->has_dst || !__libc_secs_to_tm(t + tz->std_offset, &local))
		return false;

	long long year = (long long)local.tm_year + 1900;
	long long start = change_time(&tz->start, year, tz->std_offset);
	long long end = change_time(&tz->end, year, tz->dst_offset);

	/* A start after the end means the year begins and ends in daylight saving time (southern hemisphere) */
	if (start < end)
		return t >= start && t < end;
	return t < end || t >= start;
}
