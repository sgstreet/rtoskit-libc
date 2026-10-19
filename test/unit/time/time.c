/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * time.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "rtos-linux.h"

/*
 * <time.h>, <locale.h> and strerror (P1.11, docs/research/time-locale.md):
 * calendar round trips to the limits of int years, mktime normalisation,
 * POSIX TZ rules (reference instants from the IANA zones), every strftime
 * conversion, the clocks, the "C" locale and the error messages.
 */

static int failures;

static void fail(int line, const char *what)
{
	printf("fail: line %d: %s\n", line, what);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static char tz_entry[64];
static char *environment[2];

/* Sets TZ (or unsets it for a null value) through the Linux port's environment */
static void set_tz(const char *value)
{
	environment[0] = nullptr;
	if (value != nullptr) {
		snprintf(tz_entry, sizeof(tz_entry), "TZ=%s", value);
		environment[0] = tz_entry;
	}
	__rtos_linux_environ = environment;
}

static struct tm make_tm(int year, int mon, int mday, int hour, int min, int sec, int isdst)
{
	return (struct tm){ .tm_year = year - 1900, .tm_mon = mon - 1, .tm_mday = mday, .tm_hour = hour, .tm_min = min, .tm_sec = sec, .tm_isdst = isdst };
}

/* The weekday of a day count from 1970-01-01, a Thursday */
static int weekday(long long days)
{
	return (int)(((days % 7) + 7 + 4) % 7);
}

static void check_gmtime(void)
{
	static const struct {
		long long t;
		int year;
		int mon;
		int mday;
		int hour;
		int min;
		int sec;
		int wday;
		int yday;
	} vectors[] = {
		{ 0, 1970, 0, 1, 0, 0, 0, 4, 0 },
		{ -1, 1969, 11, 31, 23, 59, 59, 3, 364 },
		{ 2147483647, 2038, 0, 19, 3, 14, 7, 2, 18 },
		{ 2147483648LL, 2038, 0, 19, 3, 14, 8, 2, 18 },
		{ 951782400, 2000, 1, 29, 0, 0, 0, 2, 59 },
		{ 4107542400LL, 2100, 2, 1, 0, 0, 0, 1, 59 },
		{ -62167219200LL, 0, 0, 1, 0, 0, 0, 6, 0 },
		{ -62198755200LL, -1, 0, 1, 0, 0, 0, 5, 0 },
		{ 253402300799LL, 9999, 11, 31, 23, 59, 59, 5, 364 },
	};
	struct tm tm;
	time_t t;

	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		t = vectors[i].t;
		CHECK(gmtime_r(&t, &tm) == &tm);
		if (tm.tm_year != vectors[i].year - 1900 || tm.tm_mon != vectors[i].mon || tm.tm_mday != vectors[i].mday || tm.tm_hour != vectors[i].hour || tm.tm_min != vectors[i].min || tm.tm_sec != vectors[i].sec || tm.tm_wday != vectors[i].wday || tm.tm_yday != vectors[i].yday || tm.tm_isdst != 0) {
			char message[128];

			snprintf(message, sizeof(message), "gmtime(%lld) = %d-%d-%d %d:%d:%d wday %d yday %d", vectors[i].t, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday, tm.tm_yday);
			fail(__LINE__, message);
		}
		CHECK(timegm(&tm) == vectors[i].t);
	}

	/* A sweep over 2^40 seconds either side of the epoch: round trips and consistent weekdays */
	for (long long s = -(1LL << 40); s <= (1LL << 40); s += 86399LL * 9973) {
		t = s;
		if (gmtime_r(&t, &tm) == nullptr || timegm(&tm) != s || tm.tm_wday != weekday((s >= 0 ? s : s - 86399) / 86400)) {
			fail(__LINE__, "gmtime/timegm sweep");
			break;
		}
	}

	/* The last and first instants whose year fits int, and one beyond each */
	tm = make_tm(0, 12, 31, 23, 59, 59, 0);
	tm.tm_year = INT_MAX;
	t = timegm(&tm);
	CHECK(t > 0 && tm.tm_year == INT_MAX);
	CHECK(gmtime(&t) != nullptr && gmtime(&t)->tm_year == INT_MAX);
	++t;
	errno = 0;
	CHECK(gmtime(&t) == nullptr && errno == EOVERFLOW);
	tm = make_tm(0, 1, 1, 0, 0, 0, 0);
	tm.tm_year = INT_MIN;
	t = timegm(&tm);
	CHECK(t < 0 && gmtime_r(&t, &tm) == &tm && tm.tm_year == INT_MIN && tm.tm_mon == 0 && tm.tm_mday == 1);
	--t;
	errno = 0;
	CHECK(gmtime_r(&t, &tm) == nullptr && errno == EOVERFLOW);
	t = LLONG_MAX;
	CHECK(gmtime(&t) == nullptr);
	t = LLONG_MIN;
	CHECK(gmtime(&t) == nullptr);

	/* gmtime and localtime share one result */
	t = 0;
	CHECK(gmtime(&t) == localtime(&t));
}

static void check_mktime(void)
{
	struct tm tm;

	set_tz("UTC0");

	/* Month 13 is January of the next year */
	tm = make_tm(2023, 13, 1, 0, 0, 0, 0);
	CHECK(mktime(&tm) == 1704067200 && tm.tm_year == 124 && tm.tm_mon == 0 && tm.tm_wday == 1 && tm.tm_yday == 0);
	/* Day 0 is the last day of the previous month, day -1 the one before */
	tm = make_tm(2024, 3, 0, 0, 0, 0, 0);
	CHECK(mktime(&tm) == 1709164800 && tm.tm_mon == 1 && tm.tm_mday == 29);
	tm = make_tm(2023, 3, -1, 0, 0, 0, 0);
	CHECK(mktime(&tm) == 1677456000 && tm.tm_mon == 1 && tm.tm_mday == 27);
	/* Hour 25, second 60 and negative minutes carry */
	tm = make_tm(2024, 12, 31, 25, -1, 60, 0);
	CHECK(mktime(&tm) == 1735693200 && tm.tm_year == 125 && tm.tm_mday == 1 && tm.tm_hour == 1 && tm.tm_min == 0 && tm.tm_sec == 0);
	/* Month -1 and huge day counts */
	tm = make_tm(2024, -1, 1, 0, 0, 0, 0);
	CHECK(mktime(&tm) == 1698796800 && tm.tm_year == 123 && tm.tm_mon == 10);
	tm = make_tm(1970, 1, 1 + 1000000, 0, 0, 0, 0);
	CHECK(mktime(&tm) == 86400000000LL && tm.tm_year == 4707 - 1900);
	/* A year beyond int fails */
	tm = make_tm(0, 1, 1, 0, 0, 0, 0);
	tm.tm_year = INT_MAX;
	tm.tm_mon = 12;
	errno = 0;
	CHECK(mktime(&tm) == (time_t)-1 && errno == EOVERFLOW);
	/* -1 can be a valid time */
	tm = make_tm(1969, 12, 31, 23, 59, 59, -1);
	errno = 0;
	CHECK(mktime(&tm) == -1 && errno == 0 && tm.tm_isdst == 0);
}

static const struct {
	const char *tz;
	long long t;
	const char *local;
	int isdst;
} zones[] = {
	{ "EST5EDT,M3.2.0,M11.1.0", 1710053999LL, "2024-03-10 01:59:59 EST -0500", 0 },
	{ "EST5EDT,M3.2.0,M11.1.0", 1710054000LL, "2024-03-10 03:00:00 EDT -0400", 1 },
	{ "EST5EDT,M3.2.0,M11.1.0", 1730613599LL, "2024-11-03 01:59:59 EDT -0400", 1 },
	{ "EST5EDT,M3.2.0,M11.1.0", 1730613600LL, "2024-11-03 01:00:00 EST -0500", 0 },
	{ "EST5EDT", 1710054000LL, "2024-03-10 03:00:00 EDT -0400", 1 },
	{ "CET-1CEST,M3.5.0,M10.5.0/3", 1711846799LL, "2024-03-31 01:59:59 CET +0100", 0 },
	{ "CET-1CEST,M3.5.0,M10.5.0/3", 1711846800LL, "2024-03-31 03:00:00 CEST +0200", 1 },
	{ "CET-1CEST,M3.5.0,M10.5.0/3", 1729990799LL, "2024-10-27 02:59:59 CEST +0200", 1 },
	{ "CET-1CEST,M3.5.0,M10.5.0/3", 1729990800LL, "2024-10-27 02:00:00 CET +0100", 0 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1704067200LL, "2024-01-01 13:00:00 NZDT +1300", 1 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1719792000LL, "2024-07-01 12:00:00 NZST +1200", 0 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1712411999LL, "2024-04-07 02:59:59 NZDT +1300", 1 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1712412000LL, "2024-04-07 02:00:00 NZST +1200", 0 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1727531999LL, "2024-09-29 01:59:59 NZST +1200", 0 },
	{ "NZST-12NZDT,M9.5.0,M4.1.0/3", 1727532000LL, "2024-09-29 03:00:00 NZDT +1300", 1 },
	{ "XST0XDT,J60/0,J300/0", 1709251199LL, "2024-02-29 23:59:59 XST +0000", 0 },
	{ "XST0XDT,J60/0,J300/0", 1709251200LL, "2024-03-01 01:00:00 XDT +0100", 1 },
	{ "XST0XDT,J60/0,J300/0", 1729983599LL, "2024-10-26 23:59:59 XDT +0100", 1 },
	{ "XST0XDT,J60/0,J300/0", 1729983600LL, "2024-10-26 23:00:00 XST +0000", 0 },
	{ "XST0XDT,59/0,300/0", 1709164799LL, "2024-02-28 23:59:59 XST +0000", 0 },
	{ "XST0XDT,59/0,300/0", 1709164800LL, "2024-02-29 01:00:00 XDT +0100", 1 },
	{ "XST0XDT,59/0,300/0", 1677628799LL, "2023-02-28 23:59:59 XST +0000", 0 },
	{ "XST0XDT,59/0,300/0", 1677628800LL, "2023-03-01 01:00:00 XDT +0100", 1 },
	{ "XST0XDT,M3.2.0/-1,M11.1.0/26", 1710025199LL, "2024-03-09 22:59:59 XST +0000", 0 },
	{ "XST0XDT,M3.2.0/-1,M11.1.0/26", 1710025200LL, "2024-03-10 00:00:00 XDT +0100", 1 },
	{ "XST0XDT,M3.2.0/-1,M11.1.0/26", 1730681999LL, "2024-11-04 01:59:59 XDT +0100", 1 },
	{ "XST0XDT,M3.2.0/-1,M11.1.0/26", 1730682000LL, "2024-11-04 01:00:00 XST +0000", 0 },
	{ "XST0XDT,M3.5.1/0,M10.5.0/0", 1711324799LL, "2024-03-24 23:59:59 XST +0000", 0 },
	{ "XST0XDT,M3.5.1/0,M10.5.0/0", 1711324800LL, "2024-03-25 01:00:00 XDT +0100", 1 },
	{ "<+0530>-5:30", 0LL, "1970-01-01 05:30:00 +0530 +0530", 0 },
	{ "<-03>3", 0LL, "1969-12-31 21:00:00 -03 -0300", 0 },
	{ "XST-1:02:03", 0LL, "1970-01-01 01:02:03 XST +0102", 0 },
	{ "UTC0", 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
	{ "GMT", 1719792000LL, "2024-07-01 00:00:00 GMT +0000", 0 },
	{ "", 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
	{ ":Europe/Paris", 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
	{ "AB3", 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
	{ "5", 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
	{ nullptr, 1719792000LL, "2024-07-01 00:00:00 UTC +0000", 0 },
};

static void check_zones(void)
{
	char text[64];
	char message[160];
	struct tm tm;

	for (size_t i = 0; i < sizeof(zones) / sizeof(zones[0]); ++i) {
		time_t t = zones[i].t;

		set_tz(zones[i].tz);
		if (localtime_r(&t, &tm) == nullptr || strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S %Z %z", &tm) == 0 || strcmp(text, zones[i].local) != 0 || tm.tm_isdst != zones[i].isdst) {
			snprintf(message, sizeof(message), "TZ=%s localtime(%lld) = \"%s\" isdst %d", zones[i].tz, zones[i].t, text, tm.tm_isdst);
			fail(__LINE__, message);
			continue;
		}
		/* Back through mktime, with the daylight saving flag localtime chose */
		if (mktime(&tm) != t) {
			snprintf(message, sizeof(message), "TZ=%s mktime(localtime(%lld))", zones[i].tz, zones[i].t);
			fail(__LINE__, message);
		}
	}

	/* tm_isdst negative: a time in the spring gap is read as standard time, an ambiguous one too */
	set_tz("EST5EDT,M3.2.0,M11.1.0");
	struct tm gap = make_tm(2024, 3, 10, 2, 30, 0, -1);

	CHECK(mktime(&gap) == 1710055800 && gap.tm_hour == 3 && gap.tm_min == 30 && gap.tm_isdst == 1);
	struct tm overlap = make_tm(2024, 11, 3, 1, 30, 0, -1);

	CHECK(mktime(&overlap) == 1730615400 && overlap.tm_hour == 1 && overlap.tm_isdst == 0);
	overlap = make_tm(2024, 11, 3, 1, 30, 0, 1);
	CHECK(mktime(&overlap) == 1730611800 && overlap.tm_isdst == 1);
	overlap = make_tm(2024, 7, 1, 12, 0, 0, -1);
	CHECK(mktime(&overlap) == 1719849600 && overlap.tm_isdst == 1);
	/* A positive flag in standard time reads the fields as daylight saving time */
	overlap = make_tm(2024, 1, 1, 12, 0, 0, 1);
	CHECK(mktime(&overlap) == 1704124800 && overlap.tm_hour == 11 && overlap.tm_isdst == 0);

	/* ctime is asctime of local time */
	time_t t = 1710054000;

	CHECK(strcmp(ctime(&t), "Sun Mar 10 03:00:00 2024\n") == 0);
	CHECK(ctime(&t) == asctime(gmtime(&t)) && strcmp(asctime(gmtime(&t)), "Sun Mar 10 07:00:00 2024\n") == 0);
	set_tz("UTC0");
}

static void check_strftime(void)
{
	static const char conversions[] = "%a|%A|%b|%B|%h|%c|%C|%d|%D|%e|%F|%g|%G|%H|%I|%j|%m|%M|%p|%r|%R|%S|%T|%u|%U|%V|%w|%W|%x|%X|%y|%Y|%%";
	static const struct {
		struct tm tm;
		const char *expected;
	} vectors[] = {
		{ { .tm_year = 124, .tm_mon = 11, .tm_mday = 30, .tm_hour = 0, .tm_min = 5, .tm_sec = 9, .tm_wday = 1, .tm_yday = 364 }, "Mon|Monday|Dec|December|Dec|Mon Dec 30 00:05:09 2024|20|30|12/30/24|30|2024-12-30|25|2025|00|12|365|12|05|AM|12:05:09 AM|00:05|09|00:05:09|1|52|01|1|53|12/30/24|00:05:09|24|2024|%" },
		{ { .tm_year = 121, .tm_mon = 0, .tm_mday = 3, .tm_hour = 23, .tm_min = 59, .tm_sec = 59, .tm_wday = 0, .tm_yday = 2 }, "Sun|Sunday|Jan|January|Jan|Sun Jan  3 23:59:59 2021|20|03|01/03/21| 3|2021-01-03|20|2020|23|11|003|01|59|PM|11:59:59 PM|23:59|59|23:59:59|7|01|53|0|00|01/03/21|23:59:59|21|2021|%" },
		{ { .tm_year = 126, .tm_mon = 11, .tm_mday = 31, .tm_hour = 12, .tm_min = 0, .tm_sec = 0, .tm_wday = 4, .tm_yday = 364 }, "Thu|Thursday|Dec|December|Dec|Thu Dec 31 12:00:00 2026|20|31|12/31/26|31|2026-12-31|26|2026|12|12|365|12|00|PM|12:00:00 PM|12:00|00|12:00:00|4|52|53|4|52|12/31/26|12:00:00|26|2026|%" },
		{ { .tm_year = 99, .tm_mon = 6, .tm_mday = 4, .tm_hour = 13, .tm_min = 7, .tm_sec = 1, .tm_wday = 0, .tm_yday = 184 }, "Sun|Sunday|Jul|July|Jul|Sun Jul  4 13:07:01 1999|19|04|07/04/99| 4|1999-07-04|99|1999|13|01|185|07|07|PM|01:07:01 PM|13:07|01|13:07:01|7|27|26|0|26|07/04/99|13:07:01|99|1999|%" },
	};
	char text[256];
	char message[400];
	struct tm tm = vectors[0].tm;

	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		size_t length = strftime(text, sizeof(text), conversions, &vectors[i].tm);

		if (length != strlen(vectors[i].expected) || strcmp(text, vectors[i].expected) != 0) {
			snprintf(message, sizeof(message), "strftime = \"%s\"", text);
			fail(__LINE__, message);
		}
	}

	/* E and O modifiers, white space, %s, an unknown conversion and a lone % */
	CHECK(strftime(text, sizeof(text), "%Ec|%EY|%Od|%OH|%n|%t|%s|%q|%", &tm) == 55 && strcmp(text, "Mon Dec 30 00:05:09 2024|2024|30|00|\n|\t|1735517109|%q|%") == 0);
	/* Out-of-range names */
	tm.tm_wday = 7;
	tm.tm_mon = -1;
	CHECK(strftime(text, sizeof(text), "%a %B", &tm) == 3 && strcmp(text, "? ?") == 0);
	tm = vectors[0].tm;

	/* The time zone is unknown with a negative tm_isdst; otherwise %z and %Z follow it */
	set_tz("EST5EDT,M3.2.0,M11.1.0");
	tm.tm_isdst = -1;
	CHECK(strftime(text, sizeof(text), "[%z%Z]", &tm) == 2 && strcmp(text, "[]") == 0);
	tm.tm_isdst = 1;
	CHECK(strftime(text, sizeof(text), "%z %Z %s", &tm) == 20 && strcmp(text, "-0400 EDT 1735531509") == 0);
	tm.tm_isdst = 0;
	CHECK(strftime(text, sizeof(text), "%z %Z", &tm) == 9 && strcmp(text, "-0500 EST") == 0);
	set_tz("UTC0");

	/* The result and its null must fit */
	CHECK(strftime(text, 11, "%F", &tm) == 10 && strcmp(text, "2024-12-30") == 0);
	CHECK(strftime(text, 10, "%F", &tm) == 0);
	CHECK(strftime(text, 0, "%F", &tm) == 0);
	CHECK(strftime(text, 1, "", &tm) == 0 && text[0] == '\0');

	/* POSIX flags and widths (the year rules libc-test checks, plus padding of the other numbers) */
	CHECK(strftime(text, sizeof(text), "%+6Y|%07F|%5d|%05d|%3e|%2a", &tm) == 37 && strcmp(text, "+02024|2024-12-30|   30|00030| 30|Mon") == 0);
	/* Negative years; Monday December 30 of year -1 starts ISO week 1 of year 0 */
	tm.tm_year = -1 - 1900;
	CHECK(strftime(text, sizeof(text), "%Y|%C|%y|%G|%g|%05Y", &tm) == 24 && strcmp(text, "-001|00|99|0000|00|-0001") == 0);
}

static void check_clocks(void)
{
	struct timespec ts;
	struct timespec later;
	time_t now;
	clock_t start;

	now = time(nullptr);
	CHECK(now > 1700000000 && time(&now) >= now);
	CHECK(timespec_get(&ts, TIME_UTC) == TIME_UTC && ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000 && ts.tv_sec >= now);
	CHECK(timespec_get(&ts, TIME_MONOTONIC) == TIME_MONOTONIC && timespec_get(&later, TIME_MONOTONIC) == TIME_MONOTONIC);
	CHECK(later.tv_sec > ts.tv_sec || (later.tv_sec == ts.tv_sec && later.tv_nsec >= ts.tv_nsec));
	CHECK(timespec_get(&ts, TIME_ACTIVE) == TIME_ACTIVE && timespec_get(&ts, TIME_THREAD_ACTIVE) == TIME_THREAD_ACTIVE);
	CHECK(timespec_get(&ts, 0) == 0 && timespec_get(&ts, 99) == 0);
	CHECK(timespec_getres(&ts, TIME_UTC) == TIME_UTC && ts.tv_sec == 0 && ts.tv_nsec > 0);
	CHECK(timespec_getres(nullptr, TIME_MONOTONIC) == TIME_MONOTONIC && timespec_getres(&ts, 99) == 0);

	/* Processor time, not calendar time: this program has used far less than a minute */
	start = clock();
	CHECK(start >= 0 && start < 60 * CLOCKS_PER_SEC);
	while (clock() == start)
		continue;
	CHECK(clock() > start);

	CHECK(difftime(10, 4) == 6.0 && difftime(LLONG_MIN, LLONG_MAX) == -0x1p64);
}

static void check_locale(void)
{
	struct lconv *lc = localeconv();

	CHECK(strcmp(setlocale(LC_ALL, nullptr), "C") == 0);
	CHECK(strcmp(setlocale(LC_ALL, ""), "C") == 0);
	CHECK(strcmp(setlocale(LC_NUMERIC, "POSIX"), "C") == 0);
	CHECK(strcmp(setlocale(LC_TIME, "C"), "C") == 0);
	CHECK(setlocale(LC_ALL, "fr_FR.UTF-8") == nullptr);
	CHECK(setlocale(99, "C") == nullptr && setlocale(-1, nullptr) == nullptr);
	CHECK(strcmp(lc->decimal_point, ".") == 0 && strcmp(lc->thousands_sep, "") == 0 && strcmp(lc->grouping, "") == 0);
	CHECK(strcmp(lc->currency_symbol, "") == 0 && strcmp(lc->int_curr_symbol, "") == 0 && strcmp(lc->mon_decimal_point, "") == 0);
	CHECK(lc->frac_digits == CHAR_MAX && lc->int_n_sign_posn == CHAR_MAX && lc->p_cs_precedes == CHAR_MAX);
	CHECK(localeconv() == lc);
}

static void check_strerror(void)
{
	static const int numbers[] = { EPERM, ENOENT, ESRCH, EINTR, EIO, ENXIO, E2BIG, ENOEXEC, EBADF, ECHILD, EAGAIN, ENOMEM, EACCES, EFAULT, EBUSY, EEXIST, EXDEV, ENODEV, ENOTDIR, EISDIR, EINVAL, ENFILE, EMFILE, ENOTTY, ETXTBSY, EFBIG, ENOSPC, ESPIPE, EROFS, EMLINK, EPIPE, EDOM, ERANGE, EDEADLK, ENAMETOOLONG, ENOLCK, ENOSYS, ENOTEMPTY, ELOOP, ENOMSG, EIDRM, ENOSTR, ENODATA, ETIME, ENOSR, ENOLINK, EPROTO, EMULTIHOP, EBADMSG, EOVERFLOW, EILSEQ, ENOTSOCK, EDESTADDRREQ, EMSGSIZE, EPROTOTYPE, ENOPROTOOPT, EPROTONOSUPPORT, EOPNOTSUPP, EAFNOSUPPORT, EADDRINUSE, EADDRNOTAVAIL, ENETDOWN, ENETUNREACH, ENETRESET, ECONNABORTED, ECONNRESET, ENOBUFS, EISCONN, ENOTCONN, ETIMEDOUT, ECONNREFUSED, EHOSTUNREACH, EALREADY, EINPROGRESS, ESTALE, EDQUOT, ECANCELED, EOWNERDEAD, ENOTRECOVERABLE };

	/* Every number has its own message */
	for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
		CHECK(strcmp(strerror(numbers[i]), "Unknown error") != 0);
		for (size_t j = 0; j < i; ++j)
			CHECK(numbers[i] != numbers[j] && strcmp(strerror(numbers[i]), strerror(numbers[j])) != 0);
	}
	CHECK(ENOTSUP == EOPNOTSUPP && EWOULDBLOCK == EAGAIN);
	CHECK(strcmp(strerror(ETIMEDOUT), "Connection timed out") == 0 && strcmp(strerror(EPERM), "Operation not permitted") == 0);
	CHECK(strcmp(strerror(-1), "Unknown error") == 0 && strcmp(strerror(4096), "Unknown error") == 0);
}

int main(void)
{
	set_tz("UTC0");
	check_gmtime();
	check_mktime();
	check_zones();
	check_strftime();
	check_clocks();
	check_locale();
	check_strerror();

	if (failures == 0)
		puts("pass: time");

	return failures == 0 ? 0 : 1;
}
