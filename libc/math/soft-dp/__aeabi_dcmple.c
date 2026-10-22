/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dcmple.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/aeabi.h"
#include "softdouble.h"

/* a <= b, 1 or 0 (IEEE 754-2019 5.11; PLAN D99). */
__LIBC_AEABI int __aeabi_dcmple(double a, double b)
{
	int order = __libc_softdouble_compare(__libc_softdouble_bits(a), __libc_softdouble_bits(b));

	return order == __LIBC_SOFTDOUBLE_LESS || order == __LIBC_SOFTDOUBLE_EQUAL;
}
