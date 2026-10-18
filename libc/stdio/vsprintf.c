/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vsprintf.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* C23 7.23.6.13: vsnprintf without a limit */
int vsprintf(char *restrict s, const char *restrict format, va_list arg)
{
	return vsnprintf(s, SIZE_MAX, format, arg);
}
