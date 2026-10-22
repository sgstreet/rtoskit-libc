/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * reference_compare.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "softdouble.h"
#include "softdp.h"

/*
 * The model of the soft-dp layer's comparisons (PLAN D101): the ordering of
 * a and b (IEEE 754-2019 5.11). NaN is unordered with everything, and the
 * zeros are equal. Every other double orders as its magnitude bits do,
 * negated for a negative sign, which is one signed integer comparison.
 * x86_64 checks it against the hardware (direct.c); the Arm cores check
 * the __aeabi_dcmp* assembly against it (assembly.c).
 */
int reference_compare(uint64_t ua, uint64_t ub)
{
	int64_t ka = (int64_t)(ua & ~__LIBC_SOFTDOUBLE_SIGN);
	int64_t kb = (int64_t)(ub & ~__LIBC_SOFTDOUBLE_SIGN);

	if (__libc_softdouble_is_nan(ua) || __libc_softdouble_is_nan(ub))
		return REFERENCE_UNORDERED;
	if ((int64_t)ua < 0)
		ka = -ka;
	if ((int64_t)ub < 0)
		kb = -kb;
	if (ka < kb)
		return REFERENCE_LESS;

	return ka == kb ? REFERENCE_EQUAL : REFERENCE_GREATER;
}
