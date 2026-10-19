/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vsscanf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

#include "internal/scan.h"

/* C23 7.23.6.14 */
int vsscanf(const char *restrict s, const char *restrict format, va_list arg)
{
	struct __libc_scan_input in = { .stream = nullptr, .string = (const unsigned char *)s, .count = 0, .from_slot = false };

	return __libc_scan(&in, format, arg);
}
