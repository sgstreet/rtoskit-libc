/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_strtoint.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>

#include "internal/stdlib.h"

/* The value of digit c in bases up to 36, or 36 when c is not a digit */
static unsigned int digit_value(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')
		return (c | 0x20) - 'a' + 10;

	return 36;
}

uintmax_t __libc_strtoint(const char *restrict nptr, char **restrict endptr, int base, bool is_signed, uintmax_t max)
{
	const unsigned char *s = (const unsigned char *)nptr;
	const unsigned char *digits;
	uintmax_t value = 0;
	uintmax_t limit;
	bool negative = false;
	bool overflow = false;

	if (base < 0 || base == 1 || base > 36) {
		if (endptr != nullptr)
			*endptr = (char *)nptr;
		errno = EINVAL;
		return 0;
	}

	/* C23 7.24.1.7p2: white space, an optional sign, then the subject sequence */
	while (isspace(*s))
		++s;
	if (*s == '+' || *s == '-')
		negative = *s++ == '-';

	/* The prefix counts only when a digit of its base follows it (p3, p5) */
	if (s[0] == '0' && (s[1] | 0x20) == 'x' && (base == 0 || base == 16) && digit_value(s[2]) < 16) {
		base = 16;
		s += 2;
	} else if (s[0] == '0' && (s[1] | 0x20) == 'b' && (base == 0 || base == 2) && digit_value(s[2]) < 2) {
		base = 2;
		s += 2;
	} else if (base == 0) {
		base = s[0] == '0' ? 8 : 10;
	}

	limit = is_signed && negative ? max + 1 : max;
	for (digits = s; digit_value(*s) < (unsigned int)base; ++s) {
		unsigned int digit = digit_value(*s);

		/* Keep consuming digits after an overflow so endptr covers the whole sequence */
		if (overflow || value > (limit - digit) / (unsigned int)base)
			overflow = true;
		else
			value = value * base + digit;
	}

	if (endptr != nullptr)
		*endptr = (char *)(s == digits ? (const unsigned char *)nptr : s);

	if (overflow) {
		errno = ERANGE;
		return is_signed && negative ? -limit : limit;
	}

	/* Negation in uintmax_t; the caller's conversion gives the value in its type */
	return negative ? -value : value;
}
