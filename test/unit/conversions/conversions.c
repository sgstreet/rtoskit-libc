/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * conversions.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Integer conversions against references written here (P3.03a): strtol,
 * strtoul, strtoll and strtoull in every base from 2 to 36 around their
 * limits, around 2^32 / 36 and 2^64 / 36 (where the speed flavour changes
 * method) and over pseudo-random values of every length, with signs,
 * prefixes, white space and trailing text, checking the value, errno and
 * the end pointer; and snprintf of 64-bit values around multiples of 10^9
 * and 2^32 in decimal, hexadecimal and octal.
 */

static int failures;

static void fail(const char *what, const char *text, int base)
{
	char message[160];

	if (failures++ < 20) {
		snprintf(message, sizeof(message), "fail: %s \"%s\" base %d\n", what, text, base);
		fputs(message, stdout);
	}
}

/* The digits of v in base, most significant first */
static void to_base(char *out, uint64_t v, unsigned int base)
{
	char digits[72];
	size_t n = 0;

	do {
		unsigned int d = (unsigned int)(v % base);

		digits[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
		v /= base;
	} while (v != 0);
	while (n > 0)
		*out++ = digits[--n];
	*out = '\0';
}

/* The reference parse: magnitude (saturated at UINT64_MAX with a flag), sign and the end of the subject sequence */
struct parse {
	uint64_t magnitude;
	bool wide;
	bool negative;
	size_t end;
};

static int digit_of(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return 99;
}

static struct parse reference(const char *text, int base)
{
	struct parse r = { 0, false, false, 0 };
	size_t i = 0;
	size_t start;

	while (text[i] == ' ' || text[i] == '\t')
		++i;
	if (text[i] == '+' || text[i] == '-')
		r.negative = text[i++] == '-';
	if (text[i] == '0' && (text[i + 1] == 'x' || text[i + 1] == 'X') && (base == 0 || base == 16) && digit_of(text[i + 2]) < 16) {
		base = 16;
		i += 2;
	} else if (base == 0) {
		base = text[i] == '0' ? 8 : 10;
	}
	start = i;
	for (; digit_of(text[i]) < base; ++i) {
		uint64_t next;

		if (r.wide || __builtin_mul_overflow(r.magnitude, (uint64_t)base, &next) || __builtin_add_overflow(next, (uint64_t)digit_of(text[i]), &next))
			r.wide = true;
		else
			r.magnitude = next;
	}
	r.end = i == start ? 0 : i;
	return r;
}

/* What a strto function of the given range must return for the parse */
static uint64_t expected_value(const struct parse *r, bool is_signed, uint64_t max, bool *range)
{
	uint64_t limit = is_signed && r->negative ? max + 1 : max;

	*range = r->wide || r->magnitude > limit;
	if (*range)
		return is_signed && r->negative ? -limit : limit;
	return r->negative ? -r->magnitude : r->magnitude;
}

static void check(const char *text, int base)
{
	struct parse r = reference(text, base);
	char *end;
	bool range;
	uint64_t want;
	long l;
	unsigned long ul;
	long long ll;
	unsigned long long ull;

	want = expected_value(&r, true, LONG_MAX, &range);
	errno = 0;
	l = strtol(text, &end, base);
	if ((unsigned long)l != (unsigned long)want || (errno == ERANGE) != range || (size_t)(end - text) != r.end)
		fail("strtol", text, base);

	want = expected_value(&r, false, ULONG_MAX, &range);
	errno = 0;
	ul = strtoul(text, &end, base);
	if (ul != (unsigned long)want || (errno == ERANGE) != range || (size_t)(end - text) != r.end)
		fail("strtoul", text, base);

	want = expected_value(&r, true, LLONG_MAX, &range);
	errno = 0;
	ll = strtoll(text, &end, base);
	if ((unsigned long long)ll != want || (errno == ERANGE) != range || (size_t)(end - text) != r.end)
		fail("strtoll", text, base);

	want = expected_value(&r, false, ULLONG_MAX, &range);
	errno = 0;
	ull = strtoull(text, &end, base);
	if (ull != want || (errno == ERANGE) != range || (size_t)(end - text) != r.end)
		fail("strtoull", text, base);
}

/* v in base with each decoration: plain, signs, white space, trailing text, one more digit */
static void check_value(uint64_t v, int base)
{
	char digits[72];
	char text[96];

	to_base(digits, v, (unsigned int)base);
	snprintf(text, sizeof(text), "%s", digits);
	check(text, base);
	snprintf(text, sizeof(text), "-%s", digits);
	check(text, base);
	snprintf(text, sizeof(text), "  +%s!", digits);
	check(text, base);
	snprintf(text, sizeof(text), "%s0", digits);
	check(text, base);
	snprintf(text, sizeof(text), "-%s%c", digits, base > 10 ? 'a' + base - 11 : '0' + base - 1);
	check(text, base);
	if (base == 16) {
		snprintf(text, sizeof(text), "0x%s", digits);
		check(text, 16);
		check(text, 0);
	}
	if (base == 8 || base == 10)
		check(text, 0);
}

static void check_strto(void)
{
	static const uint64_t points[] = {
		0,
		1,
		35,
		36,
		(UINT32_MAX / 36) - 1,
		UINT32_MAX / 36,
		(UINT32_MAX / 36) + 1,
		INT32_MAX,
		(uint64_t)INT32_MAX + 1,
		UINT32_MAX,
		(uint64_t)UINT32_MAX + 1,
		(UINT64_MAX / 36) - 1,
		UINT64_MAX / 36,
		(UINT64_MAX / 36) + 1,
		INT64_MAX,
		(uint64_t)INT64_MAX + 1,
		UINT64_MAX - 1,
		UINT64_MAX,
	};
	uint64_t r = 2026;

	for (int base = 2; base <= 36; ++base) {
		for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
			for (int delta = -2; delta <= 2; ++delta)
				check_value(points[i] + (uint64_t)(int64_t)delta, base);
		}
		/* Pseudo-random values of every length */
		for (int i = 0; i < 64; ++i) {
			r = r * 6364136223846793005ULL + 1442695040888963407ULL;
			check_value(r >> (i % 64), base);
		}
	}
	check("", 10);
	check("-", 10);
	check("0x", 16);
	check("0xg", 0);
	check("   ", 10);
}

static void reference_decimal(char *out, uint64_t v, bool negative)
{
	if (negative)
		*out++ = '-';
	to_base(out, v, 10);
}

static void check_printf(void)
{
	static const uint64_t points[] = { 0, 9, 99, 100, 999999999, 1000000000, 1000000001, UINT32_MAX, (uint64_t)UINT32_MAX + 1, 999999999999999999ULL, 1000000000000000000ULL, 10000000000000000000ULL, INT64_MAX, UINT64_MAX };
	char got[96];
	char want[96];
	uint64_t r = 7;

	for (size_t i = 0; i < sizeof(points) / sizeof(points[0]) + 200; ++i) {
		uint64_t v;

		if (i < sizeof(points) / sizeof(points[0])) {
			v = points[i];
		} else {
			r = r * 6364136223846793005ULL + 1442695040888963407ULL;
			v = r >> (i % 64);
		}
		snprintf(got, sizeof(got), "%llu", (unsigned long long)v);
		reference_decimal(want, v, false);
		if (strcmp(got, want) != 0)
			fail("printf %llu", want, 10);
		if (v <= INT64_MAX) {
			snprintf(got, sizeof(got), "%lld", -(long long)v);
			reference_decimal(want, v, v != 0);
			if (strcmp(got, want) != 0)
				fail("printf %lld", want, 10);
		}
		snprintf(got, sizeof(got), "%llx", (unsigned long long)v);
		to_base(want, v, 16);
		if (strcmp(got, want) != 0)
			fail("printf %llx", want, 16);
		snprintf(got, sizeof(got), "%llo", (unsigned long long)v);
		to_base(want, v, 8);
		if (strcmp(got, want) != 0)
			fail("printf %llo", want, 8);
		snprintf(got, sizeof(got), "%u|%x", (unsigned int)v, (unsigned int)v);
		to_base(want, (uint32_t)v, 10);
		strcat(want, "|");
		to_base(want + strlen(want), (uint32_t)v, 16);
		if (strcmp(got, want) != 0)
			fail("printf %u|%x", want, 10);
	}
	snprintf(got, sizeof(got), "%lld", (long long)INT64_MIN);
	if (strcmp(got, "-9223372036854775808") != 0)
		fail("printf INT64_MIN", got, 10);
}

int main(void)
{
	check_strto();
	check_printf();

	fputs(failures == 0 ? "conversions: pass\n" : "conversions: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
