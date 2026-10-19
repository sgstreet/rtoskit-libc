/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_decimal_set.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/decimal.h"

void __libc_decimal_set(struct __libc_decimal *d, uint64_t significand, int exponent)
{
	unsigned char reversed[20];
	int n = 0;

	d->count = 0;
	d->point = 0;
	d->truncated = false;

	for (; significand != 0; significand /= 10)
		reversed[n++] = significand % 10;
	for (d->point = n; n > 0;)
		d->digits[d->count++] = reversed[--n];
	__libc_decimal_trim(d);

	if (d->count == 0)
		return;

	/* At most 309 integer digits, or 769 significant digits for the smallest binary64 values */
	while (exponent > 0) {
		int shift = exponent < __LIBC_DECIMAL_MAX_SHIFT ? exponent : __LIBC_DECIMAL_MAX_SHIFT;

		__libc_decimal_shift_left(d, shift);
		exponent -= shift;
	}
	while (exponent < 0) {
		int shift = -exponent < __LIBC_DECIMAL_MAX_SHIFT ? -exponent : __LIBC_DECIMAL_MAX_SHIFT;

		__libc_decimal_shift_right(d, shift);
		exponent += shift;
	}
}
