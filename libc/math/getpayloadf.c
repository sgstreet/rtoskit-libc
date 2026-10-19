/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * getpayloadf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#define __STDC_WANT_IEC_60559_EXT__

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* F.10.13.1: the payload of a NaN as an integer value; -1 for anything else */
float getpayloadf(const float *x)
{
	uint32_t bits = __libc_float_bits(*x);

	/* A bit test: comparing a signaling NaN would raise invalid, and this function is quiet */
	if ((bits & ~__LIBC_FLOAT_SIGN) <= __LIBC_FLOAT_EXPONENT)
		return -1;

	return (float)(bits & (__LIBC_FLOAT_FRACTION & ~__LIBC_FLOAT_QUIET));
}
