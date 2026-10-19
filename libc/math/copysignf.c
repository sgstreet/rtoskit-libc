/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * copysignf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.11.1 */
float copysignf(float x, float y)
{
	return __libc_float_from((__libc_float_bits(x) & ~__LIBC_FLOAT_SIGN) | (__libc_float_bits(y) & __LIBC_FLOAT_SIGN));
}
