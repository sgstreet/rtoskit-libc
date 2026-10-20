/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * time.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_TIME_H
#define _INTERNAL_TIME_H

#include <time.h>

#include "internal/div64.h"

/*
 * Calendar arithmetic and the POSIX TZ rules (docs/research/time-locale.md).
 * Days count from 1970-01-01 in the proleptic Gregorian calendar.
 */

#define __LIBC_SECONDS_PER_DAY 86400

/* Days from 1970-01-01 to year-month-day (month 1-12), for any 64-bit year */
static inline long long __libc_days_from_civil(long long year, unsigned int month, unsigned int day)
{
	/* Years start on March 1, so the leap day is the last day of a year */
	year -= month <= 2;

	/* Floor division without the 64-bit division helpers (PLAN D54) */
	uint32_t year_of_era;
	long long era = __libc_floor_divmod64(year, 400, &year_of_era);
	unsigned int march_month = (month + 9) % 12;
	unsigned int day_of_year = (153 * march_month + 2) / 5 + day - 1;
	unsigned int day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;

	/* 719468 days from 0000-03-01 to 1970-01-01 */
	return era * 146097 + day_of_era - 719468;
}

/* The inverse: the civil date of a day count */
static inline void __libc_civil_from_days(long long days, long long *year, unsigned int *month, unsigned int *day)
{
	days += 719468;

	uint32_t day_of_era;
	long long era = __libc_floor_divmod64(days, 146097, &day_of_era);
	unsigned int year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
	unsigned int day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
	unsigned int march_month = (5 * day_of_year + 2) / 153;

	*day = day_of_year - (153 * march_month + 2) / 5 + 1;
	*month = march_month < 10 ? march_month + 3 : march_month - 9;
	*year = year_of_era + era * 400 + (*month <= 2);
}

static inline bool __libc_is_leap(long long year)
{
	uint32_t year_of_era;

	/* The rule repeats every 400 years */
	__libc_floor_divmod64(year, 400, &year_of_era);
	return year_of_era % 4 == 0 && (year_of_era % 100 != 0 || year_of_era == 0);
}

/* gmtime and localtime share their result (C23 7.29.3p1), one per thread (PLAN D6) */
extern thread_local struct tm __libc_time_tm;

/* asctime's algorithm, shared with ctime without the deprecation warning (C23 7.29.3.1) */
char *__libc_asctime(const struct tm *timeptr);

/*
 * Broken-down time of t seconds since the epoch (no time zone applied);
 * false with errno EOVERFLOW when the year does not fit tm_year.
 */
bool __libc_secs_to_tm(long long t, struct tm *tm);

/* Seconds since the epoch of the fields of tm read as UTC, normalising out-of-range fields */
long long __libc_tm_to_secs(const struct tm *tm);

/* A POSIX TZ rule (XBD 8.3): Jn, n or Mm.w.d, at time seconds of local time */
struct __libc_tz_rule {
	char kind;
	int day;
	int week;
	int month;
	long time;
};

#define __LIBC_TZ_NAME_MAX 15

/* The time zone in TZ; offsets are what is added to UTC to get local time */
struct __libc_tz {
	char std_name[__LIBC_TZ_NAME_MAX + 1];
	char dst_name[__LIBC_TZ_NAME_MAX + 1];
	long std_offset;
	long dst_offset;
	bool has_dst;
	struct __libc_tz_rule start;
	struct __libc_tz_rule end;
};

/* Parse TZ; anything unset, empty or invalid is UTC */
void __libc_tz_get(struct __libc_tz *tz);

/* Whether daylight saving time is in effect at t seconds since the epoch (UTC) */
bool __libc_tz_is_dst(const struct __libc_tz *tz, long long t);

#endif /* _INTERNAL_TIME_H */
