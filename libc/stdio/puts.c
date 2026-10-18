/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * puts.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <string.h>

#include "internal/file.h"

/* C23 7.23.7.9 */
int puts(const char *s)
{
	size_t length = strlen(s);

	if (__libc_file_write(stdout, s, length) != length || __libc_file_write(stdout, "\n", 1) != 1)
		return EOF;

	return 0;
}
