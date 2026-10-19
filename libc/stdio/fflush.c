/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fflush.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.5.2; for an input stream, unread input is given back as POSIX specifies */
int fflush(FILE *stream)
{
	int status;

	if (stream == nullptr)
		return __libc_stdio_flush_all(false);

	__libc_file_lock(stream);
	status = __libc_file_flush(stream);
	__libc_file_unlock(stream);

	return status;
}
