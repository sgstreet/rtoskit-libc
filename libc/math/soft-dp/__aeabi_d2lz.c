/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_d2lz.c
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
 * libgcc's version is 1.9 KB with the double arithmetic it calls. Weak, as
 * __aeabi_dmul.
 */
[[gnu::weak]] __LIBC_AEABI long long __aeabi_d2lz(double a)
{
	uint64_t bits = __libc_softdouble_bits(a);
	uint64_t magnitude;

	if (__libc_softdouble_is_nan(bits))
		return 0;
	if ((bits << 1) >= (uint64_t)(1023 + 63) << 53)
		return (int64_t)bits < 0 ? INT64_MIN : INT64_MAX;

	magnitude = __libc_softdouble_truncate(bits);
	return (int64_t)bits < 0 ? (long long)(0 - magnitude) : (long long)magnitude;
}
