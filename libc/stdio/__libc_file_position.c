/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_position.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/* The handle's position adjusted by the window: plus pending output, minus unread input and pushback */
long long __libc_file_position(FILE *stream)
{
	long long position = __rtos_seek(stream->handle, 0, SEEK_CUR);

	if (position < 0) {
		errno = (int)-position;
		return -1;
	}

	if (stream->flags & __LIBC_FILE_WRITING)
		position += stream->pos - stream->buffer;
	else if (stream->flags & __LIBC_FILE_READING)
		position -= stream->end - stream->pos;
	if (stream->flags & __LIBC_FILE_UNGOT)
		position -= 1;

	return position;
}
