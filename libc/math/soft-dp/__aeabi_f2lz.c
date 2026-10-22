/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_f2lz.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "softdouble.h"

/*
 * (long long)a, truncated toward zero (C23 6.3.1.4p1; PLAN D99). Out of
 * range the behaviour is undefined; the result saturates and NaN gives 0.
 * libgcc's version is 1.7 KB with the double arithmetic it calls.
 */
__LIBC_AEABI long long __aeabi_f2lz(float a)
{
	uint32_t bits = __libc_softdouble_float_bits(a);
	uint64_t magnitude;

	if ((bits << 1) > 0xff000000u)
		return 0;
	if ((bits << 1) >= (uint32_t)(127 + 63) << 24)
		return (int32_t)bits < 0 ? INT64_MIN : INT64_MAX;

	magnitude = __libc_softdouble_truncate_float(bits);
	return (int32_t)bits < 0 ? (long long)(0 - magnitude) : (long long)magnitude;
}
