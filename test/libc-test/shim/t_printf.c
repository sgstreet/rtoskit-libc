/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * t_printf.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>
#include <rtos/rtos-port.h>

#include "test.h"

/*
 * libc-test's README contract: format at most 512 bytes, write them to
 * standard output in one unbuffered call, mark long output as truncated and
 * set t_status. The seam write keeps the output independent of stdio.
 */
int t_printf(const char *s, ...)
{
	char buffer[512];
	va_list ap;
	int n;

	t_status = 1;

	va_start(ap);
	n = vsnprintf(buffer, sizeof(buffer), s, ap);
	va_end(ap);

	if (n < 0) {
		n = 0;
	} else if ((size_t)n >= sizeof(buffer)) {
		n = sizeof(buffer);
		buffer[n - 4] = '.';
		buffer[n - 3] = '.';
		buffer[n - 2] = '.';
		buffer[n - 1] = '\n';
	}

	return __rtos_write(1, buffer, n);
}
