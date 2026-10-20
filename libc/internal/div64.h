/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * div64.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_DIV64_H
#define _INTERNAL_DIV64_H

#include <stdint.h>

/*
 * 64-bit division by a small divisor without the compiler's 64-bit division
 * helpers, which cost 300 to 900 bytes on the 32-bit targets (PLAN D54).
 * The library's conversions only ever divide by small constants (10, 7,
 * 400, 86400, 146097). lldiv and imaxdiv still divide natively.
 */

/* n / divisor, with n % divisor in *remainder; divisor must be in [1, 2^31] */
uint64_t __libc_udivmod64(uint64_t n, uint32_t divisor, uint32_t *remainder);

/* Floor division: the quotient rounded down and a remainder in [0, divisor) */
static inline int64_t __libc_floor_divmod64(int64_t n, uint32_t divisor, uint32_t *remainder)
{
	uint64_t quotient;

	if (n >= 0)
		return (int64_t)__libc_udivmod64((uint64_t)n, divisor, remainder);

	/* -n - 1 is ~n, which cannot overflow */
	quotient = __libc_udivmod64(~(uint64_t)n, divisor, remainder);
	*remainder = divisor - 1 - *remainder;

	return -(int64_t)quotient - 1;
}

#endif /* _INTERNAL_DIV64_H */
