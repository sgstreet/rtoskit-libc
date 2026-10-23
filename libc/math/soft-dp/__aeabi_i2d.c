/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_i2d.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "internal/softdouble.h"

/* (double)a, exact (C23 6.3.1.4p2; PLAN D99). */
__LIBC_AEABI double __aeabi_i2d(int a)
{
	uint32_t magnitude = a < 0 ? 0 - (uint32_t)a : (uint32_t)a;

	return __libc_softdouble_from(__libc_softdouble_from_u32(a < 0 ? __LIBC_SOFTDOUBLE_SIGN : 0, magnitude));
}
