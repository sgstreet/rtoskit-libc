/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_decimal_shift_left.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/decimal.h"

/* Multiply by 2^shift: carries run from the last digit, the result is written right-aligned and moved down */
void __libc_decimal_shift_left(struct __libc_decimal *d, unsigned int shift)
{
	uint32_t accumulator = 0;
	int write = d->count + __LIBC_DECIMAL_SHIFT_DIGITS;

	/* One division per digit: the remainder comes from the quotient (-Os does not merge / and %) */
	for (int read = d->count - 1; read >= 0; --read) {
		accumulator += (uint32_t)d->digits[read] << shift;

		uint32_t quotient = accumulator / 10;

		d->digits[--write] = accumulator - quotient * 10;
		accumulator = quotient;
	}
	while (accumulator != 0) {
		uint32_t quotient = accumulator / 10;

		d->digits[--write] = accumulator - quotient * 10;
		accumulator = quotient;
	}

	int count = d->count + __LIBC_DECIMAL_SHIFT_DIGITS - write;

	d->point += count - d->count;
	memmove(d->digits, d->digits + write, count);

	if (count > __LIBC_DECIMAL_DIGITS) {
		for (int i = __LIBC_DECIMAL_DIGITS; i < count; ++i)
			d->truncated = d->truncated || d->digits[i] != 0;
		count = __LIBC_DECIMAL_DIGITS;
	}
	d->count = count;
	__libc_decimal_trim(d);
}
