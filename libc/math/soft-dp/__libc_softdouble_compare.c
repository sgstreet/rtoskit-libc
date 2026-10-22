/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_softdouble_compare.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "softdouble.h"

/*
 * The ordering of a and b (IEEE 754-2019 5.11): NaN is unordered with
 * everything, and the zeros are equal. Every other double orders as its
 * magnitude bits do, negated for a negative sign, which is one signed
 * integer comparison.
 */
int __libc_softdouble_compare(uint64_t ua, uint64_t ub)
{
	int64_t ka = (int64_t)(ua & ~__LIBC_SOFTDOUBLE_SIGN);
	int64_t kb = (int64_t)(ub & ~__LIBC_SOFTDOUBLE_SIGN);

	if (__libc_softdouble_is_nan(ua) || __libc_softdouble_is_nan(ub))
		return __LIBC_SOFTDOUBLE_UNORDERED;
	if ((int64_t)ua < 0)
		ka = -ka;
	if ((int64_t)ub < 0)
		kb = -kb;
	if (ka < kb)
		return __LIBC_SOFTDOUBLE_LESS;

	return ka == kb ? __LIBC_SOFTDOUBLE_EQUAL : __LIBC_SOFTDOUBLE_GREATER;
}
