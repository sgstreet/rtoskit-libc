/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fputc.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.7.3 */
int fputc(int c, FILE *stream)
{
	unsigned char byte = c;
	size_t written;

	__libc_file_lock(stream);
	written = __libc_file_write(stream, &byte, 1);
	__libc_file_unlock(stream);

	return written == 1 ? byte : EOF;
}
