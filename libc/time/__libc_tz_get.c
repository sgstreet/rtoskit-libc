/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tz_get.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "internal/time.h"

/*
 * POSIX XBD 8.3: std offset [dst [offset] [,start[/time],end[/time]]].
 * Names are three or more letters, or anything alphanumeric, + or - in <>.
 */

static const char *parse_name(const char *s, char *name)
{
	size_t n = 0;

	if (*s == '<') {
		for (++s; *s != '\0' && *s != '>'; ++s)
			if (n < __LIBC_TZ_NAME_MAX)
				name[n++] = *s;
		if (*s != '>')
			return nullptr;
		++s;
	} else {
		for (; isalpha((unsigned char)*s); ++s)
			if (n < __LIBC_TZ_NAME_MAX)
				name[n++] = *s;
		if (n < 3)
			return nullptr;
	}
	name[n] = '\0';

	return s;
}

/* [+-]hh[:mm[:ss]] in seconds; the hour may reach max_hours */
static const char *parse_time(const char *s, long *seconds, int max_hours)
{
	long sign = 1;
	long parts[3] = { 0, 0, 0 };

	if (*s == '+' || *s == '-')
		sign = *s++ == '-' ? -1 : 1;
	for (int i = 0; i < 3; ++i) {
		if (!isdigit((unsigned char)*s))
			return i == 0 ? nullptr : s - 1;
		while (isdigit((unsigned char)*s) && parts[i] <= 1000)
			parts[i] = parts[i] * 10 + (*s++ - '0');
		if (*s != ':' || i == 2)
			break;
		++s;
	}
	if (parts[0] > max_hours || parts[1] > 59 || parts[2] > 59)
		return nullptr;

	*seconds = sign * (parts[0] * 3600 + parts[1] * 60 + parts[2]);
	return s;
}

static const char *parse_number(const char *s, int *value, int low, int high)
{
	int n = 0;

	if (!isdigit((unsigned char)*s))
		return nullptr;
	while (isdigit((unsigned char)*s) && n <= high)
		n = n * 10 + (*s++ - '0');
	if (n < low || n > high)
		return nullptr;

	*value = n;
	return s;
}

static const char *parse_rule(const char *s, struct __libc_tz_rule *rule)
{
	if (*s == 'J') {
		rule->kind = 'J';
		s = parse_number(s + 1, &rule->day, 1, 365);
	} else if (*s == 'M') {
		rule->kind = 'M';
		s = parse_number(s + 1, &rule->month, 1, 12);
		if (s == nullptr || *s++ != '.')
			return nullptr;
		s = parse_number(s, &rule->week, 1, 5);
		if (s == nullptr || *s++ != '.')
			return nullptr;
		s = parse_number(s, &rule->day, 0, 6);
	} else {
		rule->kind = 'D';
		s = parse_number(s, &rule->day, 0, 365);
	}
	if (s == nullptr)
		return nullptr;

	/* The default change time is 02:00 local time */
	rule->time = 2 * 3600;
	if (*s == '/')
		s = parse_time(s + 1, &rule->time, 167);

	return s;
}

void __libc_tz_get(struct __libc_tz *tz)
{
	static const struct __libc_tz_rule us_start = { .kind = 'M', .month = 3, .week = 2, .day = 0, .time = 2 * 3600 };
	static const struct __libc_tz_rule us_end = { .kind = 'M', .month = 11, .week = 1, .day = 0, .time = 2 * 3600 };
	const char *s = getenv("TZ");
	struct __libc_tz parsed = { .has_dst = false };
	long offset;

	/* UTC unless TZ holds a valid POSIX string; ":characters" names a database zone, which there is not */
	*tz = (struct __libc_tz){ .std_name = "UTC" };
	if (s == nullptr || *s == '\0' || *s == ':')
		return;

	s = parse_name(s, parsed.std_name);
	if (s == nullptr)
		return;

	/* A name alone ("GMT") is that name at UTC; POSIX requires the offset, so this is an extension */
	if (*s == '\0') {
		strcpy(tz->std_name, parsed.std_name);
		return;
	}
	s = parse_time(s, &offset, 24);
	if (s == nullptr)
		return;
	parsed.std_offset = -offset;

	if (*s != '\0') {
		s = parse_name(s, parsed.dst_name);
		if (s == nullptr)
			return;
		parsed.has_dst = true;
		parsed.dst_offset = parsed.std_offset + 3600;
		if (*s != '\0' && *s != ',') {
			s = parse_time(s, &offset, 24);
			if (s == nullptr)
				return;
			parsed.dst_offset = -offset;
		}
		/* Without rules the changes are implementation-defined: the current US rules */
		parsed.start = us_start;
		parsed.end = us_end;
		if (*s == ',') {
			s = parse_rule(s + 1, &parsed.start);
			if (s == nullptr || *s++ != ',')
				return;
			s = parse_rule(s, &parsed.end);
			if (s == nullptr)
				return;
		}
		if (*s != '\0')
			return;
	}

	*tz = parsed;
}
