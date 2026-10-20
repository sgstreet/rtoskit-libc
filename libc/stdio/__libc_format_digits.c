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

#include <stdint.h>

#include "internal/div64.h"
#include "internal/format.h"

/* Writes the digits of value backwards ending at end; returns the first digit */
char *__libc_format_digits(char *end, uintmax_t value, char conversion)
{
	const char *table = conversion == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
	char *p = end;
	unsigned int shift;
	uint32_t small;

	switch (conversion) {
		case 'b':
		case 'B':
			shift = 1;
			break;
		case 'o':
			shift = 3;
			break;
		case 'x':
		case 'X':
		case 'p':
			shift = 4;
			break;
		default:
			/* Divide in 64 bits only while needed, without the compiler's 64-bit helpers (PLAN D54) */
			while (value > UINT32_MAX) {
				uint32_t digit;

				value = __libc_udivmod64(value, 10, &digit);
				*--p = '0' + digit;
			}
			for (small = value; small != 0; small /= 10)
				*--p = '0' + small % 10;
			return p;
	}

	for (; value != 0; value >>= shift)
		*--p = table[value & ((1U << shift) - 1)];

	return p;
}
