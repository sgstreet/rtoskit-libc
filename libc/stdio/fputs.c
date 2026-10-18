/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fputs.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <string.h>

#include "internal/file.h"

/* C23 7.23.7.4 */
int fputs(const char *restrict s, FILE *restrict stream)
{
	size_t length = strlen(s);

	return __libc_file_write(stream, s, length) == length ? 0 : EOF;
}
