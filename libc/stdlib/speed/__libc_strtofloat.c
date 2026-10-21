/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_strtofloat.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <stdbit.h>
#include <stdint.h>

#include "internal/strtofloat.h"

/* Exponents saturate here, far beyond any finite or non-zero result, so int arithmetic never overflows */
#define EXPONENT_LIMIT 100000

/* Significant digits a uint64_t accumulates exactly for the fast path and hexadecimal input */
#define DECIMAL_FAST_DIGITS 19
#define HEX_DIGITS 16

/*
 * The speed flavour's parser (PLAN D78, docs/research/conversions-speed.md)
 * is the generic one with digits tested in line, no division in Clinger's
 * fast path, and a middle path between that and Simple Decimal Conversion:
 * an approximation of w * 10^q to 128 bits with a bound on its error, used
 * in any rounding direction whenever the bound cannot change the rounding.
 *
 * 10^q = 5^q * 2^q, and 5^q = 5^(28j) * 5^r with 0 <= r < 28: 5^r is exact
 * in 64 bits and 5^(28j) is kept to 128 bits, truncated, as c * 2^-s with
 * c in [2^127, 2^128) and s = 127 - 65j + (j < 0) (floor(28j log2 5) is
 * 65j or 65j - 1 there). 25 rows cover q in [-364, 335], beyond which every
 * input of up to 19 digits overflows or rounds as a tiny value.
 */
#define COARSE_STEP 28
#define COARSE_MIN -13
#define COARSE_MAX 11

/* Bits below a 128-bit approximation of 5^q times w that may still be wrong (the error analysis is in the research note) */
#define APPROXIMATION_ERROR 10

/* 5^(28j), j in [-13, 11], normalised and truncated to 128 bits (high, low) */
static const uint64_t coarse_powers[COARSE_MAX - COARSE_MIN + 1][2] = {
	{ 0xe1afa13afbd14d6d, 0x82189c09a3a1ec21 },
	{ 0xe3e27a444d8d98b7, 0xfd1b1b2308169b25 },
	{ 0xe61acf033d1a45df, 0x6fb92487298e33bd },
	{ 0xe858ad248f5c22c9, 0xd1b3400f8f9cff68 },
	{ 0xea9c227723ee8bcb, 0x465e15a979c1cadc },
	{ 0xece53cec4a314ebd, 0xa4f8bf5635246428 },
	{ 0xef340a98172aace4, 0x86fb897116c87c34 },
	{ 0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac1 },
	{ 0xf3e2f893dec3f126, 0x5a89dba3c3efccfa },
	{ 0xf64335bcf065d37d, 0x4d4617b5ff4a16d5 },
	{ 0xf8a95fcf88747d94, 0x75a44c6397ce912a },
	{ 0xfb158592be068d2e, 0xeed6e2f0f0d56712 },
	{ 0xfd87b5f28300ca0d, 0x8bca9d6e188853fc },
	{ 0x8000000000000000, 0x0000000000000000 },
	{ 0x813f3978f8940984, 0x4000000000000000 },
	{ 0x82818f1281ed449f, 0xbff8f10e7a8921a4 },
	{ 0x83c7088e1aab65db, 0x792667c6da79e0fa },
	{ 0x850fadc09923329e, 0x03e2cf6bc604ddb0 },
	{ 0x865b86925b9bc5c2, 0x0b8a2392ba45a9b2 },
	{ 0x87aa9aff79042286, 0x90fb44d2f05d0842 },
	{ 0x88fcf317f22241e2, 0x441fece3bdf81f03 },
	{ 0x8a5296ffe33cc92f, 0x82bd6b70d99aaa6f },
	{ 0x8bab8eefb6409c1a, 0x1ad089b6c2f7548e },
	{ 0x8d07e33455637eb2, 0xdb0b487b6423e1e8 },
	{ 0x8e679c2f5e44ff8f, 0x570f09eaa7ea7648 },
};

/* 5^0 .. 5^27 */
static const uint64_t fine_powers[COARSE_STEP] = {
	0x1,
	0x5,
	0x19,
	0x7d,
	0x271,
	0xc35,
	0x3d09,
	0x1312d,
	0x5f5e1,
	0x1dcd65,
	0x9502f9,
	0x2e90edd,
	0xe8d4a51,
	0x48c27395,
	0x16bcc41e9,
	0x71afd498d,
	0x2386f26fc1,
	0xb1a2bc2ec5,
	0x3782dace9d9,
	0x1158e460913d,
	0x56bc75e2d631,
	0x1b1ae4d6e2ef5,
	0x878678326eac9,
	0x2a5a058fc295ed,
	0xd3c21bcecceda1,
	0x422ca8b0a00a425,
	0x14adf4b7320334b9,
	0x6765c793fa10079d,
};

/* The 128-bit product of a and b from 32-bit halves, which every target multiplies natively */
static void multiply(uint64_t a, uint64_t b, uint64_t *high, uint64_t *low)
{
	uint64_t a0 = (uint32_t)a;
	uint64_t a1 = a >> 32;
	uint64_t b0 = (uint32_t)b;
	uint64_t b1 = b >> 32;
	uint64_t p00 = a0 * b0;
	uint64_t p01 = a0 * b1;
	uint64_t p10 = a1 * b0;
	uint64_t middle = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;

	*low = middle << 32 | (uint32_t)p00;
	*high = a1 * b1 + (p01 >> 32) + (p10 >> 32) + (middle >> 32);
}

/* The top 128 bits of the 192-bit product of (high, low) and m, shifted left once more if that leaves the top bit clear; returns the shift */
static int multiply_top(uint64_t *high, uint64_t *low, uint64_t m)
{
	uint64_t h1, h0, l1, l0;

	multiply(*high, m, &h1, &h0);
	multiply(*low, m, &l1, &l0);
	h0 += l1;
	h1 += h0 < l1;
	if ((h1 >> 63) != 0) {
		*high = h1;
		*low = h0;
		return 0;
	}
	*high = h1 << 1 | h0 >> 63;
	*low = h0 << 1 | l0 >> 63;
	return 1;
}

/*
 * w * 10^q, w the first 19 significant digits (truncated if more followed),
 * rounded into *bits unless the approximation's error might straddle a
 * rounding boundary of the format.
 */
static bool scale(uint64_t w, int q, bool truncated, bool negative, const struct __libc_float_format *format, int rounding, uint64_t *bits)
{
	if (q < COARSE_STEP * COARSE_MIN || q >= COARSE_STEP * (COARSE_MAX + 1))
		return false;

	int j = (q - COARSE_STEP * COARSE_MIN) / COARSE_STEP + COARSE_MIN;
	int r = q - COARSE_STEP * j;
	uint64_t fine = fine_powers[r];
	unsigned int fine_shift = stdc_leading_zeros(fine);
	unsigned int w_shift = stdc_leading_zeros(w);
	uint64_t high = coarse_powers[j - COARSE_MIN][0];
	uint64_t low = coarse_powers[j - COARSE_MIN][1];

	/* (high, low) ~ 5^q * 2^k, then ~ w * 5^q * 2^k: both factors are normalised, so each product needs at most one more shift */
	int k = 127 - 65 * j + (j < 0) + (int)fine_shift - 64;

	k += multiply_top(&high, &low, fine << fine_shift);
	k += (int)w_shift - 64 + multiply_top(&high, &low, w << w_shift);

	/* Only 5^0 is exact, and the product of exact factors is exact; truncated digits add up to 1 / w <= 2^-59 of the value */
	bool exact = j == 0 && !truncated;

	/*
	 * The value lies in [approximation, approximation + error) and must not
	 * reach the next half-ulp boundary, where the bits below bit
	 * 63 - precision of high, and all of low, are zero. Nor may it start on
	 * one: digits beyond the 19th may all be zero, leaving w * 10^q exact.
	 */
	if (!exact) {
		uint64_t mask = ((uint64_t)1 << (63 - format->precision)) - 1;
		uint64_t sum = low + (j == 0 ? 0 : APPROXIMATION_ERROR);
		uint64_t below = (high & mask) + (sum < low) + (truncated ? (high >> 59) + 1 : 0);

		if (((high & mask) == 0 && low == 0) || below > mask)
			return false;
	}

	*bits = __libc_float_pack(negative, high, q - k + 64, !exact || low != 0, format, rounding);
	return true;
}

/* isdigit in the "C" locale, without the call */
static bool is_digit(unsigned char c)
{
	return (unsigned int)(c - '0') < 10;
}

static bool match(const unsigned char *s, const char *word)
{
	for (; *word != '\0'; ++s, ++word)
		if ((*s | 0x20) != *word)
			return false;

	return true;
}

static unsigned int hex_value(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f')
		return (c | 0x20) - 'a' + 10;

	return 16;
}

/* An optional exponent part: the marker, an optional sign and at least one digit, else nothing is consumed */
static const unsigned char *parse_exponent(const unsigned char *s, char marker, int *exponent)
{
	const unsigned char *p = s;
	bool negative = false;
	int value = 0;

	*exponent = 0;
	if ((*p | 0x20) != marker)
		return s;
	++p;
	if (*p == '+' || *p == '-')
		negative = *p++ == '-';
	if (!is_digit(*p))
		return s;

	for (; is_digit(*p); ++p)
		if (value < EXPONENT_LIMIT)
			value = value * 10 + (*p - '0');

	*exponent = negative ? -value : value;
	return p;
}

static int saturate(int exponent)
{
	return exponent > EXPONENT_LIMIT ? EXPONENT_LIMIT : exponent < -EXPONENT_LIMIT ? -EXPONENT_LIMIT :
																					 exponent;
}

/* C23 7.24.1.5p3: hexadecimal digits with an optional point, then an optional binary exponent; s follows "0x" */
static uint64_t parse_hex(const unsigned char *s, const unsigned char **end, bool negative, const struct __libc_float_format *format, int rounding)
{
	uint64_t significand = 0;
	unsigned int digits = 0;
	bool seen_point = false;
	bool sticky = false;
	int exponent2 = 0;
	int exponent;

	for (;; ++s) {
		unsigned int value = hex_value(*s);

		if (*s == '.' && !seen_point) {
			seen_point = true;
			continue;
		}
		if (value == 16)
			break;

		if (digits == 0 && value == 0) {
			/* Leading zeros only move the point */
			exponent2 -= seen_point ? 4 : 0;
		} else if (digits < HEX_DIGITS) {
			significand = significand << 4 | value;
			++digits;
			exponent2 -= seen_point ? 4 : 0;
		} else {
			sticky = sticky || value != 0;
			exponent2 += seen_point ? 0 : 4;
		}
		exponent2 = saturate(exponent2);
	}

	*end = parse_exponent(s, 'p', &exponent);

	return __libc_float_pack(negative, significand, saturate(exponent2 + exponent), sticky, format, rounding);
}

uint64_t __libc_strtofloat(const char *restrict nptr, char **restrict endptr, const struct __libc_float_format *format, int rounding)
{
	const unsigned char *s = (const unsigned char *)nptr;
	const unsigned char *end = s;
	const unsigned char *digits;
	uint64_t sign;
	uint64_t significand = 0;
	uint64_t bits;
	unsigned int count = 0;
	bool negative = false;
	bool seen_point = false;
	bool any_digit = false;
	int adjust = 0;
	int exponent;

	/* C23 7.24.1.5p2: white space, an optional sign, then the subject sequence */
	while (isspace(*s))
		++s;
	if (*s == '+' || *s == '-')
		negative = *s++ == '-';
	sign = negative ? __libc_float_sign(format) : 0;

	/* p3: INF or INFINITY, NAN or NAN(n-char-sequence), ignoring case; the NaN payload is not used */
	if (match(s, "inf")) {
		end = s + (match(s, "infinity") ? 8 : 3);
		bits = sign | __libc_float_infinity(format);
		goto done;
	}
	if (match(s, "nan")) {
		const unsigned char *p = s + 3;

		end = p;
		if (*p == '(') {
			while (isalnum(*++p) || *p == '_');
			if (*p == ')')
				end = p + 1;
		}
		bits = sign | __libc_float_infinity(format) | (uint64_t)1 << (format->precision - 2);
		goto done;
	}

	/* "0x" counts as a prefix only when a hexadecimal digit follows, possibly after the point */
	if (s[0] == '0' && (s[1] | 0x20) == 'x' && (hex_value(s[2]) < 16 || (s[2] == '.' && hex_value(s[3]) < 16))) {
		bits = parse_hex(s + 2, &end, negative, format, rounding);
		goto done;
	}

	/* Decimal: the first 19 significant digits are accumulated for the fast path */
	for (digits = s;; ++s) {
		if (*s == '.' && !seen_point) {
			seen_point = true;
			continue;
		}
		if (!is_digit(*s))
			break;

		unsigned int digit = *s - '0';

		any_digit = true;
		if (count == 0 && digit == 0) {
			adjust -= seen_point ? 1 : 0;
		} else if (count < DECIMAL_FAST_DIGITS) {
			significand = significand * 10 + digit;
			++count;
			adjust -= seen_point ? 1 : 0;
		} else {
			/* count saturates above the fast path limit, which is all that matters */
			count = DECIMAL_FAST_DIGITS + 1;
			adjust += seen_point ? 0 : 1;
		}
		adjust = saturate(adjust);
	}
	if (!any_digit) {
		/* No subject sequence: nothing is converted (p7) */
		end = (const unsigned char *)nptr;
		bits = 0;
		goto done;
	}

	const unsigned char *mantissa_end = s;

	end = parse_exponent(s, 'e', &exponent);

	if (significand == 0) {
		bits = sign;
		goto done;
	}

	int exponent10 = saturate(adjust + exponent);

	/*
	 * Clinger's fast path first: an exact significand and an exact power of
	 * ten need one rounding, which is cheaper than the middle path even in
	 * software floating point (measured, docs/research/conversions-speed.md).
	 */
	if (rounding == __LIBC_ROUND_NEAREST && count <= DECIMAL_FAST_DIGITS) {
		uint64_t limit = (uint64_t)1 << format->precision;

		/* Move excess powers of ten into the significand while it stays exact; below 2^precision, times ten cannot wrap */
		while (exponent10 > format->max_pow10 && significand < limit && significand * 10 <= limit) {
			significand *= 10;
			--exponent10;
		}
		if (significand <= limit && exponent10 <= format->max_pow10 && exponent10 >= -format->max_pow10) {
			bits = sign | format->exact(significand, exponent10);
			goto done;
		}
	}

	/* The middle path keeps the value: Clinger's loop only moved powers of ten into the significand */
	if (scale(significand, exponent10, count > DECIMAL_FAST_DIGITS, negative, format, rounding, &bits))
		goto done;

	bits = __libc_decimal_to_float((const char *)digits, (const char *)mantissa_end, exponent, negative, format, rounding);

done:
	if (endptr != nullptr)
		*endptr = (char *)end;

	return bits;
}
