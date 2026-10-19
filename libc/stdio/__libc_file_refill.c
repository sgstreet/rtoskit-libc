/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_refill.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

bool __libc_file_refill(FILE *stream)
{
	ptrdiff_t got = __rtos_read(stream->handle, stream->buffer, stream->size);

	stream->pos = stream->buffer;
	stream->end = stream->buffer;

	if (got < 0) {
		errno = (int)-got;
		stream->flags |= __LIBC_FILE_ERROR;
		return false;
	}
	if (got == 0) {
		stream->flags |= __LIBC_FILE_EOF;
		return false;
	}

	stream->end = stream->buffer + got;
	return true;
}
