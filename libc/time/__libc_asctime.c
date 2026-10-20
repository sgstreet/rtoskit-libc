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

#include <time.h>

#include "internal/div64.h"
#include "internal/time.h"

/*
 * The result is per thread (PLAN D6); asctime and ctime both return it (C23
 * 7.29.3p1). "Sun Sep 16 01:03:52 1973\n" and the terminator need 26
 * characters; every other result is undefined behaviour (7.29.3.1p3) and is
 * cut short to fit (PLAN D52).
 */
#define TEXT_SIZE 26

static thread_local char text[TEXT_SIZE];

static size_t put(size_t length, const char *string)
{
	while (*string != '\0' && length < TEXT_SIZE - 1)
		text[length++] = *string++;

	return length;
}

/* printf's %<width>.<precision>d for the int fields and the year */
static size_t put_number(size_t length, long long value, int width, int precision)
{
	char digits[24];
	char *start = &digits[sizeof(digits) - 1];
	unsigned long long magnitude = value < 0 ? -(unsigned long long)value : (unsigned long long)value;

	*start = '\0';
	do {
		uint32_t digit;

		magnitude = __libc_udivmod64(magnitude, 10, &digit);
		*--start = (char)('0' + digit);
	} while (magnitude != 0 || &digits[sizeof(digits) - 1] - start < precision);
	if (value < 0)
		*--start = '-';
	while (&digits[sizeof(digits) - 1] - start < width)
		*--start = ' ';

	return put(length, start);
}

/*
 * C23 7.29.3.1: "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n", written directly: snprintf
 * would link the whole formatter, floating point included (PLAN D52). Names
 * out of range show "???"; the year is computed in long long.
 */
char *__libc_asctime(const struct tm *timeptr)
{
	static const char days[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char months[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	unsigned int wday = timeptr->tm_wday;
	unsigned int mon = timeptr->tm_mon;
	size_t length = 0;

	length = put(length, wday < 7 ? days[wday] : "???");
	length = put(length, " ");
	length = put(length, mon < 12 ? months[mon] : "???");
	length = put_number(length, timeptr->tm_mday, 3, 1);
	length = put(length, " ");
	length = put_number(length, timeptr->tm_hour, 0, 2);
	length = put(length, ":");
	length = put_number(length, timeptr->tm_min, 0, 2);
	length = put(length, ":");
	length = put_number(length, timeptr->tm_sec, 0, 2);
	length = put(length, " ");
	length = put_number(length, 1900LL + timeptr->tm_year, 0, 1);
	length = put(length, "\n");
	text[length] = '\0';

	return text;
}
