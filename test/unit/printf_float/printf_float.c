/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * printf_float.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal/decimal.h"

/*
 * Floating-point conversions (P1.08, docs/research/printf-float.md):
 * - generated vectors: flags, widths, precisions, ties, extreme values and
 *   special values against glibc's output (vectors.inc);
 * - the four rounding directions through the internal decimal rounding
 *   against Python's decimal module (rounding.inc);
 * - %.17g and %a round trips through strtod for random values;
 * - strfromd/strfromf/strfroml, long double, EOVERFLOW, exact long output.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[640];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static const struct {
	const char *format;
	double value;
	const char *expected;
} vectors[] = {
#include "vectors.inc"
};

static const struct {
	double value;
	int keep;
	int rounding;
	const char *digits;
	int point;
} rounding[] = {
#include "rounding.inc"
};

static void check_vectors(void)
{
	static char buffer[512];
	char message[600];

	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		int status = snprintf(buffer, sizeof(buffer), vectors[i].format, vectors[i].value);

		if (status != (int)strlen(vectors[i].expected) || strcmp(buffer, vectors[i].expected) != 0) {
			snprintf(message, sizeof(message), "\"%s\" of %a gave \"%.200s\" (%d), want \"%.200s\"", vectors[i].format, vectors[i].value, buffer, status, vectors[i].expected);
			fail(__LINE__, message);
		}
	}
}

/* The exact magnitude of value as a decimal */
static void expand(struct __libc_decimal *d, double value)
{
	uint64_t bits;

	memcpy(&bits, &value, sizeof(bits));

	uint64_t fraction = bits & ((UINT64_C(1) << 52) - 1);
	int field = (int)(bits >> 52) & 0x7ff;

	__libc_decimal_set(d, field == 0 ? fraction : fraction | UINT64_C(1) << 52, (field == 0 ? 1 : field) - 1075);
}

static void check_rounding(void)
{
	static struct __libc_decimal d;
	char digits[__LIBC_DECIMAL_DIGITS + 1];
	char message[256];

	for (size_t i = 0; i < sizeof(rounding) / sizeof(rounding[0]); ++i) {
		expand(&d, rounding[i].value);
		__libc_decimal_round(&d, rounding[i].keep, rounding[i].value < 0, rounding[i].rounding);

		for (int j = 0; j < d.count; ++j)
			digits[j] = '0' + d.digits[j];
		digits[d.count] = '\0';

		if (strcmp(digits, rounding[i].digits) != 0 || (d.count > 0 && d.point != rounding[i].point)) {
			snprintf(message, sizeof(message), "%a keep %d rounding %d: \"%.40s\" point %d, want \"%.40s\" point %d", rounding[i].value, rounding[i].keep, rounding[i].rounding, digits, d.point, rounding[i].digits, rounding[i].point);
			fail(__LINE__, message);
		}
	}
}

/* The largest expansions fit the decimal exactly: 767 digits for the smallest subnormal, 309 integer digits */
static void check_exact(void)
{
	static struct __libc_decimal d;

	expand(&d, 0x1p-1074);
	CHECK(d.count == 751 && d.point == -323 && !d.truncated);
	expand(&d, 0x1.fffffffffffffp-1022);
	CHECK(d.count == 767 && !d.truncated);
	expand(&d, DBL_MAX);
	CHECK(d.point == 309 && !d.truncated);
}

static uint64_t state = 0x2545f4914f6cdd1dULL;

static uint64_t random64(void)
{
	state ^= state << 13;
	state ^= state >> 7;
	state ^= state << 17;
	return state;
}

static void check_round_trips(void)
{
	char buffer[64];
	char message[160];

	for (int i = 0; i < 2000; ++i) {
		uint64_t bits = random64();
		double value;

		/* Finite values only, an eighth of them subnormal */
		if (i % 8 == 0)
			bits &= (UINT64_C(1) << 52) - 1;
		if (((bits >> 52) & 0x7ff) == 0x7ff)
			bits ^= UINT64_C(1) << 62;
		memcpy(&value, &bits, sizeof(value));

		/* 17 significant digits identify every binary64 value (DBL_DECIMAL_DIG) */
		snprintf(buffer, sizeof(buffer), "%.17g", value);
		if (strtod(buffer, nullptr) != value) {
			snprintf(message, sizeof(message), "%%.17g round trip of %#" PRIx64 ": %s", bits, buffer);
			fail(__LINE__, message);
		}

		/* %a is exact */
		double back;

		snprintf(buffer, sizeof(buffer), "%a", value);
		back = strtod(buffer, nullptr);
		if (memcmp(&back, &value, sizeof(value)) != 0) {
			snprintf(message, sizeof(message), "%%a round trip of %#" PRIx64 ": %s", bits, buffer);
			fail(__LINE__, message);
		}
	}
}

static void check_misc(void)
{
	static char buffer[2048];

	/* Our %a style: subnormals normalised to a leading 1, rounding carries renormalised */
	snprintf(buffer, sizeof(buffer), "%a", 0x1p-1074);
	CHECK(strcmp(buffer, "0x1p-1074") == 0);
	snprintf(buffer, sizeof(buffer), "%A", 0x1.8p-1070);
	CHECK(strcmp(buffer, "0X1.8P-1070") == 0);
	snprintf(buffer, sizeof(buffer), "%.1a", 0x1.f8p0);
	CHECK(strcmp(buffer, "0x1.0p+1") == 0);
	/* A tie with no fraction digits goes to the even leading digit: 1.5 -> 2 */
	snprintf(buffer, sizeof(buffer), "%.0a", 0x1.8p0);
	CHECK(strcmp(buffer, "0x1p+1") == 0);
	snprintf(buffer, sizeof(buffer), "%.0a", 0x1.8p-1074 * 0x1p52);
	CHECK(strcmp(buffer, "0x1p-1021") == 0);
	snprintf(buffer, sizeof(buffer), "%.0a", 0x1.7p0);
	CHECK(strcmp(buffer, "0x1p+0") == 0);
	snprintf(buffer, sizeof(buffer), "%.2a", 0x1.008p0);
	CHECK(strcmp(buffer, "0x1.00p+0") == 0);
	snprintf(buffer, sizeof(buffer), "%.2a", 0x1.018p0);
	CHECK(strcmp(buffer, "0x1.02p+0") == 0);

	/* long double is binary64 (PLAN D7) and uses the same conversion */
	snprintf(buffer, sizeof(buffer), "%Lf|%.3Le|%Lg|%La", 1.5L, -2.0L / 3, 1e-5L, 1.0L);
	CHECK(strcmp(buffer, "1.500000|-6.667e-01|1e-05|0x1p+0") == 0);

	/* Exact long output: libc-test's %.1022f of 2^-1021 and %.0f of 2^128 */
	CHECK(snprintf(buffer, sizeof(buffer), "%.1022f", 0x1p-1021) == 1024);
	CHECK(strncmp(buffer, "0.000", 5) == 0 && strcmp(buffer + 1024 - 10, "3457031250") == 0);
	snprintf(buffer, sizeof(buffer), "%.0f", 0x1p128);
	CHECK(strcmp(buffer, "340282366920938463463374607431768211456") == 0);
	CHECK(snprintf(nullptr, 0, "%.5000f", 1.0) == 5002);
	CHECK(snprintf(nullptr, 0, "%.5000e", 1.0) == 5006);
	CHECK(snprintf(nullptr, 0, "%#.5000g", 1.0) == 5001);

	/* Results beyond INT_MAX fail with EOVERFLOW before writing (C23 7.23.6.1p15) */
	errno = 0;
	CHECK(snprintf(nullptr, 0, "%.*f", INT_MAX, 1.0) < 0 && errno == EOVERFLOW);
	errno = 0;
	CHECK(snprintf(nullptr, 0, "x%*e", INT_MAX, 1.0) < 0 && errno == EOVERFLOW);

	/* strfromd, strfromf, strfroml (C23 7.24.1.3) */
	CHECK(strfromd(buffer, sizeof(buffer), "%.3f", 2.0 / 3) == 5 && strcmp(buffer, "0.667") == 0);
	CHECK(strfromf(buffer, sizeof(buffer), "%a", 0.1f) == 13 && strcmp(buffer, "0x1.99999ap-4") == 0);
	CHECK(strfroml(buffer, 4, "%e", 1.0L) == 12 && strcmp(buffer, "1.0") == 0);
	CHECK(strfromd(buffer, sizeof(buffer), "%G", -__builtin_inf()) == 4 && strcmp(buffer, "-INF") == 0);
}

int main(void)
{
	check_vectors();
	check_rounding();
	check_exact();
	check_round_trips();
	check_misc();

	if (failures == 0)
		puts("pass: printf_float");

	return failures == 0 ? 0 : 1;
}
