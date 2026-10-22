/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_dcmpun.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/aeabi.h"
#include "softdouble.h"

/*
 * isunordered(a, b), 1 or 0 (IEEE 754-2019 5.11; PLAN D99): a NaN on
 * either side, tested on the bits alone, since the libm functions call
 * this for every isnan. Weak, as __aeabi_dmul.
 */
[[gnu::weak]] __LIBC_AEABI int __aeabi_dcmpun(double a, double b)
{
	return __libc_softdouble_is_nan(__libc_softdouble_bits(a)) || __libc_softdouble_is_nan(__libc_softdouble_bits(b));
}
