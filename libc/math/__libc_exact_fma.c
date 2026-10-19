/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exact_fma.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* 128-bit helpers: value = high * 2^64 + low */

static void multiply(uint64_t a, uint64_t b, uint64_t *high, uint64_t *low)
{
	uint64_t a0 = (uint32_t)a;
	uint64_t a1 = a >> 32;
	uint64_t b0 = (uint32_t)b;
	uint64_t b1 = b >> 32;
	uint64_t p00 = a0 * b0;
	uint64_t p01 = a0 * b1;
	uint64_t p10 = a1 * b0;
	uint64_t middle = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;

	*low = (middle << 32) | (uint32_t)p00;
	*high = a1 * b1 + (p01 >> 32) + (p10 >> 32) + (middle >> 32);
}

/* Move the leading one to bit 125, leaving two bits for a carry */
static void normalise(uint64_t *high, uint64_t *low, int *exponent)
{
	int zeros = *high != 0 ? __builtin_clzll(*high) : 64 + __builtin_clzll(*low);
	int shift = zeros - 2;

	if (shift >= 64) {
		*high = *low << (shift - 64);
		*low = 0;
	} else if (shift > 0) {
		*high = (*high << shift) | (*low >> (64 - shift));
		*low <<= shift;
	}
	*exponent -= shift;
}

/* Shift right by count, collecting the bits shifted out in *sticky */
static void shift_right(uint64_t *high, uint64_t *low, int count, bool *sticky)
{
	if (count >= 128) {
		*sticky = *high != 0 || *low != 0;
		*high = 0;
		*low = 0;
	} else if (count >= 64) {
		*sticky = *low != 0 || (count > 64 && (*high << (128 - count)) != 0);
		*low = count == 64 ? *high : *high >> (count - 64);
		*high = 0;
	} else if (count > 0) {
		*sticky = (*low << (64 - count)) != 0;
		*low = (*low >> count) | (*high << (64 - count));
		*high >>= count;
	}
}

/*
 * Both terms are normalised to bit 125. The product has at most 124
 * significant bits and the addend 62, so at least 20 low bits of each are
 * zero: the smaller term loses bits only when shifted by more than 20,
 * and then the difference keeps its leading one at bit 124 or above, so the
 * sticky fraction stays far below the bits the rounding looks at.
 */
bool __libc_exact_fma(struct __libc_exact *result, bool product_negative, uint64_t mx, uint64_t my, int product_exponent, bool addend_negative, uint64_t mz, int addend_exponent)
{
	uint64_t large_high;
	uint64_t large_low;
	uint64_t small_high = 0;
	uint64_t small_low = mz;
	int large_exponent = product_exponent;
	bool large_negative = product_negative;
	bool small_negative = addend_negative;
	bool sticky = false;

	multiply(mx, my, &large_high, &large_low);
	normalise(&large_high, &large_low, &large_exponent);

	if (mz != 0) {
		normalise(&small_high, &small_low, &addend_exponent);
		if (addend_exponent > large_exponent || (addend_exponent == large_exponent && (small_high > large_high || (small_high == large_high && small_low > large_low)))) {
			uint64_t high = large_high;
			uint64_t low = large_low;
			int exponent = large_exponent;
			bool negative = large_negative;

			large_high = small_high;
			large_low = small_low;
			large_exponent = addend_exponent;
			large_negative = small_negative;
			small_high = high;
			small_low = low;
			addend_exponent = exponent;
			small_negative = negative;
		}
		shift_right(&small_high, &small_low, large_exponent - addend_exponent, &sticky);

		if (large_negative == small_negative) {
			large_low += small_low;
			large_high += small_high + (large_low < small_low);
		} else {
			/* Subtracting a sticky fraction too: one unit less, and a fraction (1 - fraction) above it */
			uint64_t subtrahend_low = small_low + sticky;
			uint64_t subtrahend_high = small_high + (subtrahend_low < small_low);

			large_high -= subtrahend_high + (large_low < subtrahend_low);
			large_low -= subtrahend_low;
		}
	}

	if (large_high == 0 && large_low == 0 && !sticky)
		return false;

	*result = (struct __libc_exact){ .negative = large_negative, .sticky = sticky, .exponent = large_exponent, .high = large_high, .low = large_low };
	return true;
}
