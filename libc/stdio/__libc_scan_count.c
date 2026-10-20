/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_scan_count.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/scan.h"

/* n (C23 7.23.6.2p12): the characters read so far, stored as an integer conversion stores */
bool __libc_scan_count(va_list *ap, enum __libc_length length, size_t count)
{
	return __libc_scan_store(ap, length, count);
}
