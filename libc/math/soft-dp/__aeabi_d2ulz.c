/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_d2ulz.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "softdouble.h"

/*
 * (unsigned long long)a, truncated toward zero (C23 6.3.1.4p1; PLAN D99).
 * Out of range the behaviour is undefined; the result saturates, a negative
 * value and NaN give 0.
 */
__LIBC_AEABI unsigned long long __aeabi_d2ulz(double a)
{
	uint64_t bits = __libc_softdouble_bits(a);

	if (__libc_softdouble_is_nan(bits) || (int64_t)bits < 0)
		return 0;
	if (bits >= (uint64_t)(1023 + 64) << 52)
		return UINT64_MAX;

	return __libc_softdouble_truncate(bits);
}
