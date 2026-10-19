/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vfscanf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

#include "internal/file.h"
#include "internal/scan.h"

/* C23 7.23.6.9: the whole call holds the stream's lock; the first unmatched character stays pushed back */
int vfscanf(FILE *restrict stream, const char *restrict format, va_list arg)
{
	struct __libc_scan_input in = { .stream = stream, .string = nullptr, .count = 0, .from_slot = false };
	int status;

	__libc_file_before_input(stream);
	__libc_file_lock(stream);
	status = __libc_scan(&in, format, arg);
	__libc_file_unlock(stream);

	return status;
}
