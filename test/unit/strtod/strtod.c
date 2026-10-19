/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtod.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal/strtofloat.h"

/*
 * strtod, strtof, strtold, atof (P1.07, docs/research/strtod.md):
 * - fixed cases: special values, hexadecimal, end pointers, errno;
 * - for random binary64 and binary32 values (subnormals included), the exact
 *   decimal expansion of the value and of the midpoint to its successor,
 *   and the midpoint nudged up and down, in every rounding direction (the
 *   directed ones through the internal core). The expansions are computed
 *   here with a decimal multiplier, independently of the library.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[512];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static uint64_t double_bits(double value)
{
	uint64_t bits;

	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static uint32_t float_bits(float value)
{
	uint32_t bits;

	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

/* Check a binary64 conversion: the bits, how much was consumed, and errno */
static void check_double(int line, const char *text, uint64_t want, int consumed, int want_errno)
{
	char *end;
	char message[256];

	errno = 0;
	uint64_t got = double_bits(strtod(text, &end));
	int got_errno = errno;

	if (got != want || end != text + consumed || got_errno != want_errno) {
		snprintf(message, sizeof(message), "strtod(\"%.80s\") = %#" PRIx64 " end %d errno %d, want %#" PRIx64 " end %d errno %d", text, got, (int)(end - text), got_errno, want, consumed, want_errno);
		fail(line, message);
	}
}

static void check_float(int line, const char *text, uint32_t want, int consumed, int want_errno)
{
	char *end;
	char message[256];

	errno = 0;
	uint32_t got = float_bits(strtof(text, &end));
	int got_errno = errno;

	if (got != want || end != text + consumed || got_errno != want_errno) {
		snprintf(message, sizeof(message), "strtof(\"%.80s\") = %#" PRIx32 " end %d errno %d, want %#" PRIx32 " end %d errno %d", text, got, (int)(end - text), got_errno, want, consumed, want_errno);
		fail(line, message);
	}
}

#define DOUBLE(text, want, consumed, want_errno) check_double(__LINE__, text, want, consumed, want_errno)
#define FLOAT(text, want, consumed, want_errno) check_float(__LINE__, text, want, consumed, want_errno)

static void fixed_cases(void)
{
	/* Zero, signs, white space, and no subject sequence (C23 7.24.1.5p7) */
	DOUBLE("0", 0, 1, 0);
	DOUBLE("  \t\n-0", 0x8000000000000000, 6, 0);
	DOUBLE("-.00000", 0x8000000000000000, 7, 0);
	DOUBLE("+0e+999999999", 0, 13, 0);
	DOUBLE("", 0, 0, 0);
	DOUBLE("+", 0, 0, 0);
	DOUBLE(" .", 0, 0, 0);
	DOUBLE("-.e1", 0, 0, 0);
	DOUBLE("x1", 0, 0, 0);

	/* Ordinary values, through the fast path and beyond it */
	DOUBLE("1", 0x3ff0000000000000, 1, 0);
	DOUBLE("1.5", 0x3ff8000000000000, 3, 0);
	DOUBLE("-2.5e-1", 0xbfd0000000000000, 7, 0);
	DOUBLE("0.1", 0x3fb999999999999a, 3, 0);
	DOUBLE("3.141592653589793", 0x400921fb54442d18, 17, 0);
	DOUBLE("9007199254740993", 0x4340000000000000, 16, 0);
	DOUBLE("1e23", 0x44b52d02c7e14af6, 4, 0);
	DOUBLE("8.98846567431158e307", 0x7fe0000000000000, 20, 0);
	DOUBLE("123456789012345678901234567890", 0x45f8ee90ff6c373e, 30, 0);
	DOUBLE("1.7976931348623157e308", 0x7fefffffffffffff, 22, 0);

	/* An exponent part needs a digit, else it is not part of the subject */
	DOUBLE("1e", 0x3ff0000000000000, 1, 0);
	DOUBLE("1e+", 0x3ff0000000000000, 1, 0);
	DOUBLE("1E-x", 0x3ff0000000000000, 1, 0);
	DOUBLE("2.e1z", 0x4034000000000000, 4, 0);

	/* Infinity and NaN, ignoring case; the n-char-sequence needs its closing parenthesis */
	DOUBLE("inf", 0x7ff0000000000000, 3, 0);
	DOUBLE("-INFINITY", 0xfff0000000000000, 9, 0);
	DOUBLE("Infinit", 0x7ff0000000000000, 3, 0);
	DOUBLE("nan", 0x7ff8000000000000, 3, 0);
	DOUBLE("-NaN", 0xfff8000000000000, 4, 0);
	DOUBLE("nan()", 0x7ff8000000000000, 5, 0);
	DOUBLE("nan(abc_123)x", 0x7ff8000000000000, 12, 0);
	DOUBLE("nan(abc", 0x7ff8000000000000, 3, 0);
	DOUBLE("nan(a-b)", 0x7ff8000000000000, 3, 0);
	DOUBLE("in", 0, 0, 0);

	/* Hexadecimal (p3): the prefix needs a hexadecimal digit after it */
	DOUBLE("0x1p4", 0x4030000000000000, 5, 0);
	DOUBLE("0X1.1P4", 0x4031000000000000, 7, 0);
	DOUBLE("0x.8", 0x3fe0000000000000, 4, 0);
	DOUBLE("-0x10", 0xc030000000000000, 5, 0);
	DOUBLE("0x", 0, 1, 0);
	DOUBLE("0x.p1", 0, 1, 0);
	DOUBLE("0xg", 0, 1, 0);
	DOUBLE("0x1p", 0x3ff0000000000000, 3, 0);
	DOUBLE("0x0.0000000000001p-1022", 0x0000000000000001, 23, 0);
	DOUBLE("0x1.fffffffffffffp1023", 0x7fefffffffffffff, 22, 0);
	DOUBLE("0x1.111111111111281", 0x3ff1111111111113, 19, 0);
	DOUBLE("0x1.11111111111111", 0x3ff1111111111111, 18, 0);
	DOUBLE("0x1.00000000000008", 0x3ff0000000000000, 18, 0);
	DOUBLE("0x1.00000000000018", 0x3ff0000000000002, 18, 0);
	DOUBLE("0x1.000000000000080000000000000001", 0x3ff0000000000001, 34, 0);
	DOUBLE("0x0000000000000000000001p0", 0x3ff0000000000000, 26, 0);

	/* Overflow and underflow (p10, 7.12.1): HUGE_VAL, and tiny inexact results */
	DOUBLE("1e309", 0x7ff0000000000000, 5, ERANGE);
	DOUBLE("-1e+1000000", 0xfff0000000000000, 11, ERANGE);
	DOUBLE("0x1.fffffffffffff8p1023", 0x7ff0000000000000, 23, ERANGE);
	DOUBLE("1e-400", 0, 6, ERANGE);
	DOUBLE("-1e-1000000", 0x8000000000000000, 11, ERANGE);
	DOUBLE("4.9e-324", 0x0000000000000001, 8, ERANGE);
	DOUBLE("0x1p-1074", 0x0000000000000001, 9, 0);
	DOUBLE("0x1p-1075", 0, 9, ERANGE);
	DOUBLE("0x1.8p-1075", 0x0000000000000001, 11, ERANGE);
	DOUBLE("0x1.ffffffffffffep-1023", 0x000fffffffffffff, 23, 0);
	DOUBLE("0x1.fffffffffffffp-1023", 0x0010000000000000, 23, ERANGE);
	DOUBLE("2.2250738585072014e-308", 0x0010000000000000, 23, 0);
	CHECK(strtod("1e309", nullptr) == HUGE_VAL);

	/* binary32: its own rounding, never through binary64 */
	FLOAT("1", 0x3f800000, 1, 0);
	FLOAT("0.1", 0x3dcccccd, 3, 0);
	FLOAT("16777217", 0x4b800000, 8, 0);
	FLOAT("16777219", 0x4b800002, 8, 0);
	FLOAT("3.4028235e38", 0x7f7fffff, 12, 0);
	FLOAT("3.40282356e38", 0x7f7fffff, 13, 0);
	FLOAT("3.5e38", 0x7f800000, 6, ERANGE);
	FLOAT("1e-46", 0, 5, ERANGE);
	FLOAT("1.4e-45", 0x00000001, 7, ERANGE);
	FLOAT("0x1p-149", 0x00000001, 8, 0);
	FLOAT("-nan", 0xffc00000, 4, 0);
	FLOAT("infinity", 0x7f800000, 8, 0);
	/* Just above 1 + 2^-24: rounding to binary64 first gives exactly 1 + 2^-24, a binary32 tie that would go to 1 */
	FLOAT("1.00000005960464477550", 0x3f800001, 22, 0);
	CHECK(strtof("3.5e38", nullptr) == HUGE_VALF);

	/* strtold is strtod (PLAN D7); atof is strtod without the end pointer */
	char *end;

	CHECK(strtold("0x1.8p1", &end) == 3.0L && *end == '\0');
	CHECK(atof(" -12.5e-1x") == -1.25);

	/* 40000 digits (libc-test strtod_long) and a long run of zeros before the first digit */
	static char text[40002];

	memset(text, '1', sizeof(text) - 1);
	text[0] = '.';
	text[sizeof(text) - 1] = '\0';
	CHECK(strtod(text, &end) == 0x1.c71c71c71c71cp-4 && end == text + sizeof(text) - 1);
	memset(text, '0', sizeof(text) - 1);
	text[1] = '.';
	text[sizeof(text) - 2] = '3';
	CHECK(strtod(text, nullptr) == 0);
	/* "0.000...3e<n>" with the exponent cancelling all but one of the zeros: 0.3 */
	snprintf(text + sizeof(text) - 12, 12, "3e%d", (int)sizeof(text) - 14);
	CHECK(strtod(text, nullptr) == 3e-1);
}

/*
 * Decimal expansions: an unsigned integer as a little-endian array of
 * decimal digits, multiplied in place by small factors.
 */
struct big {
	int count;
	unsigned char digit[800];
};

static void big_set(struct big *b, uint64_t value)
{
	b->count = 0;
	do {
		b->digit[b->count++] = value % 10;
		value /= 10;
	} while (value != 0);
}

/* factor <= 2^28, so a digit times it plus the carry fits 32 bits (no 64-bit division on 32-bit targets) */
static void big_multiply(struct big *b, uint32_t factor)
{
	uint32_t carry = 0;

	for (int i = 0; i < b->count; ++i) {
		carry += b->digit[i] * factor;
		b->digit[i] = carry % 10;
		carry /= 10;
	}
	while (carry != 0) {
		b->digit[b->count++] = carry % 10;
		carry /= 10;
	}
}

/* b * 2^power or b * 5^power, by the largest factors big_multiply takes */
static void big_power(struct big *b, uint32_t base, int power)
{
	uint32_t step = base == 2 ? 1u << 28 : 244140625u;
	int per_step = base == 2 ? 28 : 12;

	for (; power >= per_step; power -= per_step)
		big_multiply(b, step);
	for (; power > 0; --power)
		big_multiply(b, base);
}

/* significand * 2^exponent = b * 10^exponent10 exactly, with b an integer */
static int expand(struct big *b, uint64_t significand, int exponent)
{
	big_set(b, significand);
	if (exponent >= 0) {
		big_power(b, 2, exponent);
		return 0;
	}

	/* significand * 2^-n = significand * 5^n * 10^-n */
	big_power(b, 5, -exponent);
	return exponent;
}

/*
 * Write b * 10^exponent10 as "<digits>e<exponent10>"; with nudge -1 or +1,
 * the value one tenth of a unit of the last digit below or above it, with
 * nudge 2 a unit of the 801st digit after the last one above it (dropped on
 * input, kept as the truncated flag), and with nudge 3 a unit in the 800th
 * significant digit above it (kept on input, but pushed beyond the 800
 * digits by the slow path's shifts for values far from 1). The integer form
 * makes the nudge a matter of digits.
 */
static void render(char *text, size_t size, const struct big *value, int exponent10, int nudge)
{
	struct big b = *value;
	char *p = text;

	if (nudge < 0) {
		/* Subtract one from the integer, then append ".9" */
		for (int i = 0; b.digit[i]-- == 0; ++i)
			b.digit[i] = 9;
		while (b.count > 1 && b.digit[b.count - 1] == 0)
			--b.count;
	}
	for (int i = b.count - 1; i >= 0; --i)
		*p++ = '0' + b.digit[i];
	if (nudge != 0) {
		int zeros = nudge == 2 ? __LIBC_DECIMAL_DIGITS : nudge == 3 ? __LIBC_DECIMAL_DIGITS - b.count - 1 :
																	  0;

		*p++ = '.';
		memset(p, '0', zeros);
		p += zeros;
		*p++ = nudge < 0 ? '9' : '1';
	}
	snprintf(p, size - (p - text), "e%d", exponent10);
}

/* The test format for the internal core; the fast path is never taken outside round-to-nearest */
static const struct __libc_float_format test_binary64 = { .precision = 53, .emin = -1022, .emax = 1023, .max_pow10 = 22, .exact = nullptr };
static const struct __libc_float_format test_binary32 = { .precision = 24, .emin = -126, .emax = 127, .max_pow10 = 10, .exact = nullptr };

static uint64_t state = 0x853c49e6748fea9bULL;

static uint64_t random64(void)
{
	state = state * 6364136223846793005ULL + 1442695040888963407ULL;
	return (state >> 32) ^ (state << 32 >> 5) ^ (state >> 17);
}

/*
 * One value given by its format bits (positive, finite, below the largest
 * value): its expansion converts back to it in every direction; the midpoint
 * to its successor rounds to even when nearest, and the nudged midpoints
 * round down and up.
 */
static void check_value(const struct __libc_float_format *format, uint64_t bits)
{
	/* Room for a sign, 769 digits, the point, 800 zeros and the exponent */
	static char text[2048];
	static struct big exact;
	static struct big midpoint;
	unsigned int fraction_bits = format->precision - 1;
	uint64_t fraction = bits & (((uint64_t)1 << fraction_bits) - 1);
	int field = (int)(bits >> fraction_bits);
	uint64_t significand = field == 0 ? fraction : fraction | (uint64_t)1 << fraction_bits;
	int exponent = (field == 0 ? 1 : field) + format->emin - 1 - (int)fraction_bits;
	int exact_exponent10 = expand(&exact, significand, exponent);
	int midpoint_exponent10 = expand(&midpoint, 2 * significand + 1, exponent - 1);
	bool binary64 = format->precision == 53;
	char message[160];

	for (int which = 0; which < 6; ++which) {
		/* which: 0 exact, 1 midpoint, 2 below it, 3 above it, 4 and 5 barely above it */
		static const int nudges[] = { 0, 0, -1, 1, 2, 3 };
		bool below_nearest = which == 0 || which == 2 || (which == 1 && (bits & 1) == 0);

		if (which == 0)
			render(text + 1, sizeof(text) - 1, &exact, exact_exponent10, 0);
		else
			render(text + 1, sizeof(text) - 1, &midpoint, midpoint_exponent10, nudges[which]);

		/* The same digits, positive and negative */
		for (int negative = 0; negative <= 1; ++negative) {
			uint64_t s = negative ? __libc_float_sign(format) : 0;
			uint64_t below = s | bits;
			uint64_t above = s | (bits + 1);
			const char *t = text + 1;

			if (negative) {
				text[0] = '-';
				t = text;
			}

			uint64_t want = below_nearest ? below : above;
			uint64_t got = binary64 ? double_bits(strtod(t, nullptr)) : float_bits(strtof(t, nullptr));

			if (got != want) {
				snprintf(message, sizeof(message), "nearest %#" PRIx64 " want %#" PRIx64 " for %.60s", got, want, t);
				fail(__LINE__, message);
			}

			/* Directed rounding: toward zero and toward the side of the sign keep the smaller magnitude */
			for (int rounding = 0; rounding <= 3; rounding += rounding == 0 ? 2 : 1) {
				bool larger = which != 0 && (rounding == __LIBC_ROUND_UPWARD ? !negative : rounding == __LIBC_ROUND_DOWNWARD && negative);

				want = larger ? above : below;
				got = __libc_strtofloat(t, nullptr, format, rounding);
				if (got != want) {
					snprintf(message, sizeof(message), "rounding %d %#" PRIx64 " want %#" PRIx64 " for %.60s", rounding, got, want, t);
					fail(__LINE__, message);
				}
			}
		}
	}
}

static void random_values(const struct __libc_float_format *format, int count)
{
	unsigned int width = format->precision == 53 ? 63 : 31;
	uint64_t infinity = __libc_float_infinity(format);

	for (int i = 0; i < count; ++i) {
		uint64_t bits = random64() & (((uint64_t)1 << width) - 1);

		/* Every eighth value is subnormal, and the binades at the ends get extra weight */
		if (i % 8 == 0)
			bits &= ((uint64_t)1 << (format->precision - 1)) - 1;
		else if (i % 8 == 1)
			bits = (bits & (((uint64_t)1 << (format->precision + 1)) - 1)) | (i % 16 == 1 ? infinity - ((uint64_t)2 << (format->precision - 1)) : 0);
		if (bits >= infinity - 1)
			bits = infinity - 2;

		check_value(format, bits);
	}

	/* The boundaries themselves */
	check_value(format, 0);
	check_value(format, 1);
	check_value(format, ((uint64_t)1 << (format->precision - 1)) - 1);
	check_value(format, (uint64_t)1 << (format->precision - 1));
	check_value(format, infinity - 2);
}

/* Directed overflow (F.5): infinity or the largest value, by direction and sign */
static void directed_overflow(void)
{
	static const struct {
		const char *text;
		int rounding;
		uint64_t want;
		int want_errno;
	} cases[] = {
		{ "1e400", __LIBC_ROUND_TOWARD_ZERO, 0x7fefffffffffffff, ERANGE },
		{ "1e400", __LIBC_ROUND_UPWARD, 0x7ff0000000000000, ERANGE },
		{ "1e400", __LIBC_ROUND_DOWNWARD, 0x7fefffffffffffff, ERANGE },
		{ "-1e400", __LIBC_ROUND_UPWARD, 0xffefffffffffffff, ERANGE },
		{ "-1e400", __LIBC_ROUND_DOWNWARD, 0xfff0000000000000, ERANGE },
		{ "1e-400", __LIBC_ROUND_UPWARD, 0x0000000000000001, ERANGE },
		{ "1e-400", __LIBC_ROUND_DOWNWARD, 0, ERANGE },
		{ "-1e-400", __LIBC_ROUND_DOWNWARD, 0x8000000000000001, ERANGE },
		{ "-1e-400", __LIBC_ROUND_TOWARD_ZERO, 0x8000000000000000, ERANGE },
		{ "0x1.fffffffffffff8p1023", __LIBC_ROUND_TOWARD_ZERO, 0x7fefffffffffffff, 0 },
		{ "0x1.fffffffffffff8p1023", __LIBC_ROUND_UPWARD, 0x7ff0000000000000, ERANGE },
		{ "1.5", __LIBC_ROUND_DOWNWARD, 0x3ff8000000000000, 0 },
		{ "0.1", __LIBC_ROUND_DOWNWARD, 0x3fb9999999999999, 0 },
		{ "0.1", __LIBC_ROUND_UPWARD, 0x3fb999999999999a, 0 },
		{ "-0.1", __LIBC_ROUND_TOWARD_ZERO, 0xbfb9999999999999, 0 },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		errno = 0;
		uint64_t got = __libc_strtofloat(cases[i].text, nullptr, &test_binary64, cases[i].rounding);

		if (got != cases[i].want) {
			char message[128];

			snprintf(message, sizeof(message), "%s rounding %d: %#" PRIx64 " want %#" PRIx64, cases[i].text, cases[i].rounding, got, cases[i].want);
			fail(__LINE__, message);
		}
		CHECK(errno == cases[i].want_errno);
	}
}

int main(void)
{
	fixed_cases();
	directed_overflow();
	random_values(&test_binary64, 48);
	random_values(&test_binary32, 48);

	if (failures == 0)
		puts("pass: strtod");

	return failures == 0 ? 0 : 1;
}
