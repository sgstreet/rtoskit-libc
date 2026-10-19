/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vprintf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

/* C23 7.23.6.10 */
int vprintf(const char *restrict format, va_list arg)
{
	return vfprintf(stdout, format, arg);
}
