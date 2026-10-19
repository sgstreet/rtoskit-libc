/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_scan.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "internal/scan.h"

/*
 * C23 7.23.6.2 (docs/research/scanf.md). Every field reads characters while
 * they still form a prefix of a matching sequence and pushes back only the
 * first one that does not, the one character of pushback a stream allows.
 */

/* Length modifiers (p11), kept so each store uses the exact type */
enum length {
	LENGTH_NONE,
	LENGTH_HH,
	LENGTH_H,
	LENGTH_L,
	LENGTH_LL,
	LENGTH_J,
	LENGTH_Z,
	LENGTH_T,
	LENGTH_W8,
	LENGTH_W16,
	LENGTH_W32,
	LENGTH_W64,
	LENGTH_WF8,
	LENGTH_WF16,
	LENGTH_WF32,
	LENGTH_WF64,
	LENGTH_BIG_L,
};

/* The outcome of one directive */
enum result {
	RESULT_OK,
	RESULT_MATCHING_FAILURE,
	RESULT_INPUT_FAILURE,
};

/* No field width: limited only by size_t */
#define NO_WIDTH SIZE_MAX

static bool parse_length(const char **format, enum length *length)
{
	const char *s = *format;

	switch (*s) {
		case 'h':
			*length = s[1] == 'h' ? LENGTH_HH : LENGTH_H;
			s += s[1] == 'h' ? 2 : 1;
			break;
		case 'l':
			*length = s[1] == 'l' ? LENGTH_LL : LENGTH_L;
			s += s[1] == 'l' ? 2 : 1;
			break;
		case 'j':
			*length = LENGTH_J;
			++s;
			break;
		case 'z':
			*length = LENGTH_Z;
			++s;
			break;
		case 't':
			*length = LENGTH_T;
			++s;
			break;
		case 'L':
			*length = LENGTH_BIG_L;
			++s;
			break;
		case 'w': {
			/* C23 wN and wfN: N is 8, 16, 32 or 64 */
			bool fast = s[1] == 'f';
			int bits = 0;

			s += fast ? 2 : 1;
			if (*s < '1' || *s > '9')
				return false;
			while (*s >= '0' && *s <= '9' && bits <= 64)
				bits = bits * 10 + (*s++ - '0');
			switch (bits) {
				case 8:
					*length = fast ? LENGTH_WF8 : LENGTH_W8;
					break;
				case 16:
					*length = fast ? LENGTH_WF16 : LENGTH_W16;
					break;
				case 32:
					*length = fast ? LENGTH_WF32 : LENGTH_W32;
					break;
				case 64:
					*length = fast ? LENGTH_WF64 : LENGTH_W64;
					break;
				default:
					return false;
			}
			break;
		}
		default:
			*length = LENGTH_NONE;
			break;
	}

	*format = s;
	return true;
}

/* Converts value to the type the modifier names; out-of-range values wrap as the conversion does */
static void store_integer(va_list *ap, enum length length, uintmax_t value)
{
	switch (length) {
		case LENGTH_HH:
			*va_arg(*ap, unsigned char *) = (unsigned char)value;
			break;
		case LENGTH_H:
			*va_arg(*ap, unsigned short *) = (unsigned short)value;
			break;
		case LENGTH_L:
			*va_arg(*ap, unsigned long *) = (unsigned long)value;
			break;
		case LENGTH_LL:
			*va_arg(*ap, unsigned long long *) = (unsigned long long)value;
			break;
		case LENGTH_J:
			*va_arg(*ap, uintmax_t *) = value;
			break;
		case LENGTH_Z:
			*va_arg(*ap, size_t *) = (size_t)value;
			break;
		case LENGTH_T:
			*va_arg(*ap, ptrdiff_t *) = (ptrdiff_t)value;
			break;
		case LENGTH_W8:
			*va_arg(*ap, uint_least8_t *) = (uint_least8_t)value;
			break;
		case LENGTH_W16:
			*va_arg(*ap, uint_least16_t *) = (uint_least16_t)value;
			break;
		case LENGTH_W32:
			*va_arg(*ap, uint_least32_t *) = (uint_least32_t)value;
			break;
		case LENGTH_W64:
			*va_arg(*ap, uint_least64_t *) = (uint_least64_t)value;
			break;
		case LENGTH_WF8:
			*va_arg(*ap, uint_fast8_t *) = (uint_fast8_t)value;
			break;
		case LENGTH_WF16:
			*va_arg(*ap, uint_fast16_t *) = (uint_fast16_t)value;
			break;
		case LENGTH_WF32:
			*va_arg(*ap, uint_fast32_t *) = (uint_fast32_t)value;
			break;
		case LENGTH_WF64:
			*va_arg(*ap, uint_fast64_t *) = (uint_fast64_t)value;
			break;
		default:
			*va_arg(*ap, unsigned int *) = (unsigned int)value;
			break;
	}
}

static unsigned int digit_value(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')
		return (c | 0x20) - 'a' + 10;

	return 36;
}

/* p8: input white space is skipped before every conversion except [, c and n */
static int skip_space(struct __libc_scan_input *in)
{
	int c;

	do
		c = __libc_scan_get(in);
	while (c != EOF && isspace(c));
	__libc_scan_unget(in, c);

	return c;
}

/*
 * d, i, o, u, x, X, b, B, p (p12): an optional sign, the base's optional
 * prefix (0x for 16, 0b for 2; for base 0 they pick the base, and a leading 0
 * means 8), then digits, as strtol's subject sequence. A prefix with no digit
 * after it is a matching failure.
 */
static enum result scan_integer(struct __libc_scan_input *in, size_t width, unsigned int base, uintmax_t *value)
{
	uintmax_t result = 0;
	size_t used = 0;
	bool negative = false;
	bool digits = false;
	int c = __libc_scan_get(in);

	if (c == EOF)
		return RESULT_INPUT_FAILURE;

	if ((c == '+' || c == '-') && used < width) {
		negative = c == '-';
		++used;
		c = __libc_scan_get(in);
	}

	if (c == '0' && used < width) {
		/* A leading zero is a digit in every base */
		digits = true;
		++used;
		c = __libc_scan_get(in);
		if (used < width && (c | 0x20) == 'x' && (base == 0 || base == 16)) {
			base = 16;
			digits = false;
			++used;
			c = __libc_scan_get(in);
		} else if (used < width && (c | 0x20) == 'b' && (base == 0 || base == 2)) {
			base = 2;
			digits = false;
			++used;
			c = __libc_scan_get(in);
		} else if (base == 0) {
			base = 8;
		}
	} else if (base == 0) {
		base = 10;
	}

	while (used < width && digit_value(c) < base) {
		result = result * base + digit_value(c);
		digits = true;
		++used;
		c = __libc_scan_get(in);
	}
	__libc_scan_unget(in, c);

	if (!digits)
		return RESULT_MATCHING_FAILURE;

	*value = negative ? -result : result;
	return RESULT_OK;
}

/* p12 [: the characters between the brackets, a leading ^ inverting the set; returns the format after the ] */
static const char *parse_scanset(const char *s, unsigned char set[32])
{
	bool invert = false;
	unsigned char bits[32] = {};

	if (*s == '^') {
		invert = true;
		++s;
	}
	/* A ] right after [ or [^ is part of the set */
	if (*s == ']') {
		bits[']' / 8] |= 1 << (']' % 8);
		++s;
	}
	while (*s != '\0' && *s != ']') {
		unsigned char first = *s;

		/* a-z is a range when - is neither first nor last (implementation-defined, p12) */
		if (s[1] == '-' && s[2] != ']' && s[2] != '\0' && (unsigned char)s[2] >= first) {
			for (unsigned int c = first; c <= (unsigned char)s[2]; ++c)
				bits[c / 8] |= 1 << (c % 8);
			s += 3;
		} else {
			bits[first / 8] |= 1 << (first % 8);
			++s;
		}
	}

	for (unsigned int i = 0; i < 32; ++i)
		set[i] = invert ? ~bits[i] : bits[i];

	return s;
}

static bool in_set(const unsigned char set[32], int c)
{
	return (set[(unsigned char)c / 8] >> ((unsigned char)c % 8)) & 1;
}

int __libc_scan(struct __libc_scan_input *in, const char *format, va_list ap)
{
	va_list args;
	int assigned = 0;
	int completed = 0;
	enum result result = RESULT_OK;

	va_copy(args, ap);

	while (*format != '\0' && result == RESULT_OK) {
		unsigned char f = *format;

		/* p5: white space matches any amount of input white space, none included */
		if (isspace(f)) {
			while (isspace((unsigned char)*format))
				++format;
			skip_space(in);
			continue;
		}

		/* p6: an ordinary character must match the next input character */
		if (f != '%' || format[1] == '%') {
			int c;

			if (f == '%') {
				/* %% skips white space like the other conversions (p8) */
				skip_space(in);
				format += 2;
			} else {
				++format;
			}
			c = __libc_scan_get(in);
			if (c != f) {
				__libc_scan_unget(in, c);
				result = c == EOF ? RESULT_INPUT_FAILURE : RESULT_MATCHING_FAILURE;
			}
			continue;
		}

		/* A conversion specification: *, width, length, conversion */
		bool suppress = false;
		size_t width = NO_WIDTH;
		enum length length;

		++format;
		if (*format == '*') {
			suppress = true;
			++format;
		}
		if (*format >= '1' && *format <= '9') {
			width = 0;
			while (*format >= '0' && *format <= '9') {
				size_t digit = *format++ - '0';

				width = width > (NO_WIDTH - 1 - digit) / 10 ? NO_WIDTH - 1 : width * 10 + digit;
			}
		}
		if (!parse_length(&format, &length)) {
			errno = EINVAL;
			break;
		}

		char conversion = *format++;
		unsigned int base = 10;

		switch (conversion) {
			case 'n':
				/* p12: no input consumed, not counted as an assignment */
				if (!suppress)
					store_integer(&args, length, in->count);
				continue;

			case 'c':
			case 's':
			case '[': {
				/* Wide characters are out of scope (PLAN §1) */
				if (length != LENGTH_NONE) {
					errno = EILSEQ;
					result = RESULT_MATCHING_FAILURE;
					continue;
				}

				unsigned char set[32];
				char *target = suppress ? nullptr : va_arg(args, char *);
				size_t count = 0;
				bool end = false;
				int c;

				if (conversion == '[') {
					format = parse_scanset(format, set);
					if (*format != ']') {
						errno = EINVAL;
						result = RESULT_MATCHING_FAILURE;
						continue;
					}
					++format;
				} else if (conversion == 's') {
					skip_space(in);
				}
				if (conversion == 'c' && width == NO_WIDTH)
					width = 1;

				while (count < width) {
					c = __libc_scan_get(in);
					if (c == EOF) {
						end = true;
						break;
					}
					if ((conversion == 's' && isspace(c)) || (conversion == '[' && !in_set(set, c))) {
						__libc_scan_unget(in, c);
						break;
					}
					if (target != nullptr)
						target[count] = c;
					++count;
				}

				/* %c needs exactly width characters, %s and %[ at least one; running out of input is an input failure */
				if (count == 0 || (conversion == 'c' && count < width)) {
					result = end ? RESULT_INPUT_FAILURE : RESULT_MATCHING_FAILURE;
					continue;
				}
				if (target != nullptr && conversion != 'c')
					target[count] = '\0';
				++completed;
				assigned += target != nullptr;
				continue;
			}

			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G': {
				char kind = length == LENGTH_L ? 'l' : length == LENGTH_BIG_L ? 'L' :
																				0;

				if (length != LENGTH_NONE && length != LENGTH_L && length != LENGTH_BIG_L) {
					errno = EINVAL;
					result = RESULT_MATCHING_FAILURE;
					continue;
				}
				if (skip_space(in) == EOF) {
					result = RESULT_INPUT_FAILURE;
					continue;
				}

				int status = __libc_scan_float(in, width, kind, suppress ? nullptr : va_arg(args, void *));

				if (status != 1) {
					result = status == EOF ? RESULT_INPUT_FAILURE : RESULT_MATCHING_FAILURE;
					continue;
				}
				++completed;
				assigned += !suppress;
				continue;
			}

			case 'd':
				base = 10;
				break;
			case 'i':
				base = 0;
				break;
			case 'o':
				base = 8;
				break;
			case 'u':
				base = 10;
				break;
			case 'x':
			case 'X':
			case 'p':
				base = 16;
				break;
			case 'b':
			case 'B':
				base = 2;
				break;
			default:
				/* An invalid conversion specification: undefined (p13); stop */
				errno = EINVAL;
				result = RESULT_MATCHING_FAILURE;
				continue;
		}

		/* The integer conversions */
		uintmax_t value;

		if (skip_space(in) == EOF) {
			result = RESULT_INPUT_FAILURE;
			continue;
		}
		result = scan_integer(in, width, base, &value);
		if (result != RESULT_OK)
			continue;
		if (!suppress) {
			if (conversion == 'p')
				*va_arg(args, void **) = (void *)(uintptr_t)value;
			else
				store_integer(&args, length, value);
			++assigned;
		}
		++completed;
	}

	va_end(args);

	/* p16: EOF when an input failure comes before the first conversion */
	if (result == RESULT_INPUT_FAILURE && completed == 0)
		return EOF;

	return assigned;
}
