/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_getc.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdio.h>

#include "internal/file.h"

/* Turning from output to input writes the pending output first */
static bool begin_input(FILE *stream)
{
	if (!(stream->flags & __LIBC_FILE_READ)) {
		errno = EBADF;
		stream->flags |= __LIBC_FILE_ERROR;
		return false;
	}
	if ((stream->flags & __LIBC_FILE_WRITING) && __libc_file_flush(stream) != 0)
		return false;
	__libc_file_setup(stream);
	stream->flags |= __LIBC_FILE_READING;

	return true;
}

int __libc_file_getc(FILE *stream)
{
	if (stream->flags & __LIBC_FILE_UNGOT) {
		stream->flags &= ~__LIBC_FILE_UNGOT;
		return stream->ungot;
	}
	if ((stream->flags & __LIBC_FILE_READING) && stream->pos < stream->end)
		return *stream->pos++;

	/* C23 7.23.7.1p3: once the end-of-file indicator is set, fgetc returns EOF */
	if ((stream->flags & __LIBC_FILE_EOF) || !begin_input(stream) || !__libc_file_refill(stream))
		return EOF;

	return *stream->pos++;
}
