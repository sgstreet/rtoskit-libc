/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_ui2d.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "softdouble.h"

/* (double)a, exact (C23 6.3.1.4p2; PLAN D99). Weak, as __aeabi_dmul. */
[[gnu::weak]] __LIBC_AEABI double __aeabi_ui2d(unsigned int a)
{
	return __libc_softdouble_from(__libc_softdouble_from_u32(0, a));
}
