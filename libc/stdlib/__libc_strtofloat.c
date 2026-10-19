/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_strtofloat.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <stdint.h>

#include "internal/strtofloat.h"

/* Exponents saturate here, far beyond any finite or non-zero result, so int arithmetic never overflows */
#define EXPONENT_LIMIT 100000

/* Significant digits a uint64_t accumulates exactly for the fast path and hexadecimal input */
#define DECIMAL_FAST_DIGITS 19
#define HEX_DIGITS 16

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
	if (!isdigit(*p))
		return s;

	for (; isdigit(*p); ++p)
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
		if (!isdigit(*s))
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

	/* Clinger's fast path: an exact significand and an exact power of ten need one rounding */
	int exponent10 = saturate(adjust + exponent);

	if (rounding == __LIBC_ROUND_NEAREST && count <= DECIMAL_FAST_DIGITS) {
		uint64_t limit = (uint64_t)1 << format->precision;

		/* Move excess powers of ten into the significand while it stays exact */
		while (exponent10 > format->max_pow10 && significand <= limit / 10) {
			significand *= 10;
			--exponent10;
		}
		if (significand <= limit && exponent10 <= format->max_pow10 && exponent10 >= -format->max_pow10) {
			bits = sign | format->exact(significand, exponent10);
			goto done;
		}
	}

	bits = __libc_decimal_to_float((const char *)digits, (const char *)mantissa_end, exponent, negative, format, rounding);

done:
	if (endptr != nullptr)
		*endptr = (char *)end;

	return bits;
}
