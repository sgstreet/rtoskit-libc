/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_decimal_shift_right.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/decimal.h"

/* Divide by 2^shift: a sliding accumulator reads digits in and writes quotient digits out */
void __libc_decimal_shift_right(struct __libc_decimal *d, unsigned int shift)
{
	uint32_t mask = ((uint32_t)1 << shift) - 1;
	uint32_t accumulator = 0;
	int read = 0;
	int write = 0;

	/* Read until the first quotient digit is non-zero, past the end with zeros if needed */
	while ((accumulator >> shift) == 0) {
		accumulator = accumulator * 10 + (read < d->count ? d->digits[read] : 0);
		++read;
	}
	d->point -= read - 1;

	/* The write position stays behind the read position */
	while (read < d->count) {
		unsigned char digit = accumulator >> shift;

		accumulator = (accumulator & mask) * 10 + d->digits[read++];
		d->digits[write++] = digit;
	}

	while (accumulator != 0) {
		unsigned char digit = accumulator >> shift;

		accumulator = (accumulator & mask) * 10;
		if (write < __LIBC_DECIMAL_DIGITS)
			d->digits[write++] = digit;
		else if (digit != 0)
			d->truncated = true;
	}

	d->count = write;
	__libc_decimal_trim(d);
}
