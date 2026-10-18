/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vsnprintf.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

#include "internal/format.h"

/* C23 7.23.6.12 */
int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list arg)
{
	struct __libc_format_buffer buffer = {
		.sink = { .write = __libc_format_buffer_write },
		.pos = s,
		.room = n > 0 ? n - 1 : 0,
	};
	int status = __libc_format(&buffer.sink, format, arg);

	/* With n zero nothing is written and s may be a null pointer */
	if (n > 0)
		*buffer.pos = '\0';

	return status;
}
