/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_udivmod64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/div64.h"

/*
 * Where registers are 64 bits wide the division is native. Elsewhere the
 * high word divides with a 32-bit division, then the low word's bits are
 * brought down one at a time; the running remainder stays below the divisor,
 * at most 2^31, so shifting it left cannot overflow 32 bits (PLAN D54).
 */
uint64_t __libc_udivmod64(uint64_t n, uint32_t divisor, uint32_t *remainder)
{
	if (sizeof(uintptr_t) >= sizeof(uint64_t)) {
		*remainder = (uint32_t)(n % divisor);
		return n / divisor;
	}

	uint32_t high = (uint32_t)(n >> 32);
	uint32_t low = (uint32_t)n;
	uint32_t rest = high % divisor;
	uint64_t quotient = (uint64_t)(high / divisor) << 32;

	for (int bit = 31; bit >= 0; --bit) {
		rest = rest << 1 | (low >> bit & 1);
		if (rest >= divisor) {
			rest -= divisor;
			quotient |= (uint64_t)1 << bit;
		}
	}

	*remainder = rest;
	return quotient;
}
