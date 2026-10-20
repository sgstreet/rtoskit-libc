/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_digits.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/format.h"

/*
 * libc-minimal-printf.a: no argument is wider than unsigned long
 * (__libc_format_argument refuses the rest), so the digits come from long
 * division only, which 32-bit cores do without 64-bit library helpers
 * (PLAN D53).
 */
char *__libc_format_digits(char *end, uintmax_t value, char conversion)
{
	const char *table = conversion == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
	unsigned long small = (unsigned long)value;
	unsigned int base;
	char *p = end;

	switch (conversion) {
		case 'b':
		case 'B':
			base = 2;
			break;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
		case 'p':
			base = 16;
			break;
		default:
			base = 10;
			break;
	}

	for (; small != 0; small /= base)
		*--p = table[small % base];

	return p;
}
