/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_decimal_round.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/decimal.h"

/*
 * The digit at index keep and everything after it decide the rounding: below,
 * at or above half a unit of the last kept digit. When keep <= 0 the whole
 * value lies below half a unit, except that keep == 0 compares the leading
 * digit with half.
 */
void __libc_decimal_round(struct __libc_decimal *d, int keep, bool negative, int rounding)
{
	bool increment;
	bool above_half;
	bool half;

	if (keep >= d->count) {
		/* Nothing is dropped, so the value is exact */
		return;
	}

	if (keep < 0) {
		half = false;
		above_half = false;
	} else {
		bool tail = d->truncated;

		for (int i = keep + 1; i < d->count && !tail; ++i)
			tail = d->digits[i] != 0;
		half = d->digits[keep] == 5 && !tail;
		above_half = d->digits[keep] > 5 || (d->digits[keep] == 5 && tail);
	}
	/* count > keep and the digits are trimmed, so a non-zero digit is dropped: the result is inexact */
	switch (rounding) {
		case __LIBC_ROUND_TOWARD_ZERO:
			increment = false;
			break;
		case __LIBC_ROUND_UPWARD:
			increment = !negative;
			break;
		case __LIBC_ROUND_DOWNWARD:
			increment = negative;
			break;
		default:
			/* Ties to even; a tie above every kept digit (keep == 0) rounds to zero */
			increment = above_half || (half && keep > 0 && (d->digits[keep - 1] & 1) != 0);
			break;
	}

	d->truncated = false;
	if (keep <= 0) {
		/* Either zero or one unit at the position just above the kept digits */
		if (increment) {
			d->point -= keep - 1;
			d->digits[0] = 1;
			d->count = 1;
		} else {
			d->count = 0;
		}
		return;
	}

	d->count = keep;
	if (increment) {
		int i = keep - 1;

		while (i >= 0 && d->digits[i] == 9)
			d->digits[i--] = 0;
		if (i >= 0) {
			++d->digits[i];
		} else {
			/* 99...9 + 1: a new leading digit */
			d->digits[0] = 1;
			d->count = 1;
			++d->point;
		}
	}
	__libc_decimal_trim(d);
}
