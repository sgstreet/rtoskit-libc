/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_asctime.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <time.h>

#include "internal/time.h"

/* The result is per thread (PLAN D6); asctime and ctime both return it (C23 7.29.3p1) */
static thread_local char text[64];

/*
 * C23 7.29.3.1: "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n". Fields outside the
 * ranges are undefined behaviour; names show "???" and the buffer is sized
 * for any int values.
 */
char *__libc_asctime(const struct tm *timeptr)
{
	static const char days[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char months[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	unsigned int wday = timeptr->tm_wday;
	unsigned int mon = timeptr->tm_mon;

	snprintf(text, sizeof(text), "%.3s %.3s%3d %.2d:%.2d:%.2d %lld\n", wday < 7 ? days[wday] : "???", mon < 12 ? months[mon] : "???", timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, 1900LL + timeptr->tm_year);

	return text;
}
