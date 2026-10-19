/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * nextupf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.11.5 */
float nextupf(float x)
{
	uint32_t bits = __libc_float_bits(x);

	if (isnan(x))
		return x + x;
	if ((bits & ~__LIBC_FLOAT_SIGN) == 0)
		return __libc_float_from(1);
	if (bits == __LIBC_FLOAT_EXPONENT)
		return x;

	return __libc_float_from((bits & __LIBC_FLOAT_SIGN) != 0 ? bits - 1 : bits + 1);
}
