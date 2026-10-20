/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_scan_float.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>

#include "internal/scan.h"

/*
 * libc-nofloat.a: a/A/e/E/f/F/g/G read nothing and are a matching failure
 * with errno EINVAL, as an invalid conversion is (D41), so strtod's
 * conversion stays out of a program that scans only integers and strings
 * (PLAN D13, D53).
 */
int __libc_scan_float([[maybe_unused]] struct __libc_scan_input *in, [[maybe_unused]] size_t width, [[maybe_unused]] char length, [[maybe_unused]] void *target)
{
	errno = EINVAL;
	return 0;
}
