/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * snprintf.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

/* C23 7.23.6.5 */
int snprintf(char *restrict s, size_t n, const char *restrict format, ...)
{
	va_list ap;
	int status;

	va_start(ap);
	status = vsnprintf(s, n, format, ap);
	va_end(ap);

	return status;
}
