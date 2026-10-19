/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_flush.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/*
 * Pending output is written out. Unread input (and a pushed-back character)
 * is given back by moving the handle's position back over it, as POSIX does
 * for fflush on an input stream; where the handle cannot seek, it is dropped.
 * Either way the window is empty and has no direction afterwards.
 */
int __libc_file_flush(FILE *stream)
{
	int status = 0;

	if (stream->flags & __LIBC_FILE_WRITING) {
		const unsigned char *next = stream->buffer;

		while (next < stream->pos) {
			ptrdiff_t written = __rtos_write(stream->handle, next, stream->pos - next);

			if (written <= 0) {
				/* The rest of the pending output is lost */
				errno = written < 0 ? (int)-written : EIO;
				stream->flags |= __LIBC_FILE_ERROR;
				status = EOF;
				break;
			}
			next += written;
		}
	} else if (stream->flags & (__LIBC_FILE_READING | __LIBC_FILE_UNGOT)) {
		long long unread = ((stream->flags & __LIBC_FILE_READING) ? stream->end - stream->pos : 0) + ((stream->flags & __LIBC_FILE_UNGOT) ? 1 : 0);

		if (unread > 0)
			__rtos_seek(stream->handle, -unread, SEEK_CUR);
	}

	stream->flags &= ~(__LIBC_FILE_READING | __LIBC_FILE_WRITING | __LIBC_FILE_UNGOT);
	stream->pos = stream->buffer;
	stream->end = stream->buffer;

	return status;
}
