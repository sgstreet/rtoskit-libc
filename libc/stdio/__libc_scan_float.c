/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_scan_float.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal/scan.h"

/*
 * C23 7.23.6.2p12 a/e/f/g: strtod's subject sequence, read as the longest
 * prefix of one (docs/research/scanf.md). The number is kept in a normal
 * form, significant digits only with the point folded into the exponent,
 * so input of any length converts correctly rounded.
 */

/* Significant digits kept: more than the 800 strtod's slow path keeps; the rest only matter as a sticky digit */
#define MAX_DIGITS 900

/* Exponents saturate far beyond any finite or non-zero result */
#define EXPONENT_LIMIT 100000

static int saturate(long value)
{
	return value > EXPONENT_LIMIT ? EXPONENT_LIMIT : value < -EXPONENT_LIMIT ? -EXPONENT_LIMIT :
																			   (int)value;
}

static unsigned int digit_value(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f')
		return (c | 0x20) - 'a' + 10;

	return 16;
}

/* Appends the decimal digits of value */
static char *append_number(char *p, int value)
{
	char digits[12];
	unsigned int magnitude = value < 0 ? -(unsigned int)value : (unsigned int)value;
	int n = 0;

	if (value < 0)
		*p++ = '-';
	do {
		digits[n++] = '0' + magnitude % 10;
		magnitude /= 10;
	} while (magnitude != 0);
	while (n > 0)
		*p++ = digits[--n];

	return p;
}

/* INF or INFINITY, NAN or NAN(n-char-sequence), ignoring case; c is the first letter */
static bool scan_special(struct __libc_scan_input *in, size_t width, size_t *used, int *next, bool *nan)
{
	int c = *next;
	const char *word = (c | 0x20) == 'i' ? "infinity" : "nan";
	size_t matched = 0;
	bool ok;

	*nan = word[0] == 'n';
	while (word[matched] != '\0' && *used < width && (c | 0x20) == word[matched]) {
		++matched;
		++*used;
		c = __libc_scan_get(in);
	}

	/* "infin" is a prefix of a matching sequence but not one itself */
	ok = matched == 3 || matched == 8;
	if (ok && *nan && c == '(' && *used < width) {
		do {
			++*used;
			c = __libc_scan_get(in);
		} while (*used < width && (isalnum(c) || c == '_'));
		ok = c == ')';
		if (ok) {
			++*used;
			c = __libc_scan_get(in);
		}
	}

	*next = c;
	return ok;
}

int __libc_scan_float(struct __libc_scan_input *in, size_t width, char length, void *target)
{
	char digits[MAX_DIGITS];
	char text[MAX_DIGITS + 32];
	char *p = text;
	size_t used = 0;
	size_t count = 0;
	long point = 0;
	long exponent = 0;
	bool any_digit = false;
	bool seen_point = false;
	bool sticky = false;
	bool hex = false;
	int c = __libc_scan_get(in);

	if (c == EOF)
		return EOF;

	if ((c == '+' || c == '-') && used < width) {
		if (c == '-')
			*p++ = '-';
		++used;
		c = __libc_scan_get(in);
	}

	if (used < width && ((c | 0x20) == 'i' || (c | 0x20) == 'n')) {
		bool nan;
		bool ok = scan_special(in, width, &used, &c, &nan);

		__libc_scan_unget(in, c);
		if (!ok)
			return 0;
		for (const char *s = nan ? "nan" : "inf"; *s != '\0'; ++s)
			*p++ = *s;
		*p = '\0';
	} else {
		if (c == '0' && used < width) {
			any_digit = true;
			++used;
			c = __libc_scan_get(in);
			if ((c | 0x20) == 'x' && used < width) {
				/* "0x" alone is a prefix, not a matching sequence */
				hex = true;
				any_digit = false;
				++used;
				c = __libc_scan_get(in);
			}
		}

		/* The significand: digits and at most one point */
		while (used < width) {
			unsigned int digit = digit_value(c);

			if (c == '.' && !seen_point) {
				seen_point = true;
			} else if (digit < (hex ? 16u : 10u)) {
				any_digit = true;
				if (count == 0 && digit == 0) {
					/* Leading zeros only move the point */
					point -= seen_point;
				} else {
					if (count < MAX_DIGITS)
						digits[count++] = c;
					else
						sticky = sticky || digit != 0;
					point += !seen_point;
				}
			} else {
				break;
			}
			++used;
			c = __libc_scan_get(in);
		}

		/* The exponent: the marker, an optional sign and at least one digit */
		if (any_digit && used < width && (c | 0x20) == (hex ? 'p' : 'e')) {
			bool negative = false;
			bool exponent_digit = false;

			++used;
			c = __libc_scan_get(in);
			if ((c == '+' || c == '-') && used < width) {
				negative = c == '-';
				++used;
				c = __libc_scan_get(in);
			}
			while (used < width && c >= '0' && c <= '9') {
				exponent_digit = true;
				exponent = saturate(exponent * 10 + (c - '0'));
				++used;
				c = __libc_scan_get(in);
			}
			if (!exponent_digit) {
				__libc_scan_unget(in, c);
				return 0;
			}
			exponent = negative ? -exponent : exponent;
		}
		__libc_scan_unget(in, c);

		if (!any_digit)
			return 0;

		/* [-]0.<digits>[1]e<exp>, or [-]0x0.<digits>[1]p<exp> with the point counted in bits */
		if (count == 0) {
			*p++ = '0';
		} else {
			for (const char *s = hex ? "0x0." : "0."; *s != '\0'; ++s)
				*p++ = *s;
			for (size_t i = 0; i < count; ++i)
				*p++ = digits[i];
			if (sticky)
				*p++ = '1';
			*p++ = hex ? 'p' : 'e';
			p = append_number(p, saturate(exponent + (hex ? 4 * point : point)));
		}
		*p = '\0';
	}

	if (target != nullptr) {
		if (length == 'l')
			*(double *)target = strtod(text, nullptr);
		else if (length == 'L')
			*(long double *)target = strtold(text, nullptr);
		else
			*(float *)target = strtof(text, nullptr);
	}

	return 1;
}
