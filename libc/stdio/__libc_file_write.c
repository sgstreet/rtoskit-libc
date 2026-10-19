/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_write.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>
#include <string.h>

#include "internal/file.h"

/* Straight to the handle; returns the bytes written before any error */
static size_t write_through(FILE *stream, const unsigned char *data, size_t count)
{
	size_t done = 0;

	while (done < count) {
		ptrdiff_t written = __rtos_write(stream->handle, data + done, count - done);

		if (written <= 0) {
			errno = written < 0 ? (int)-written : EIO;
			stream->flags |= __LIBC_FILE_ERROR;
			break;
		}
		done += written;
	}

	return done;
}

size_t __libc_file_write(FILE *stream, const void *buffer, size_t count)
{
	const unsigned char *data = buffer;
	size_t done = 0;

	if (!(stream->flags & __LIBC_FILE_WRITE)) {
		errno = EBADF;
		stream->flags |= __LIBC_FILE_ERROR;
		return 0;
	}

	/* Turning from input to output gives the unread input back first */
	if ((stream->flags & __LIBC_FILE_READING) && __libc_file_flush(stream) != 0)
		return 0;
	__libc_file_setup(stream);
	stream->flags |= __LIBC_FILE_WRITING;

	if (stream->flags & __LIBC_FILE_UNBUFFERED)
		return write_through(stream, data, count);

	while (done < count) {
		size_t room = stream->buffer + stream->size - stream->pos;

		/* Large writes bypass an empty buffer */
		if (stream->pos == stream->buffer && count - done >= stream->size)
			return done + write_through(stream, data + done, count - done);

		if (room == 0) {
			if (__libc_file_flush(stream) != 0)
				return done;
			stream->flags |= __LIBC_FILE_WRITING;
			continue;
		}

		size_t chunk = count - done < room ? count - done : room;

		memcpy(stream->pos, data + done, chunk);
		stream->pos += chunk;
		done += chunk;
	}

	/* C23 7.23.3p3: a line-buffered stream transmits when a new-line is written */
	if ((stream->flags & __LIBC_FILE_LINE) && memchr(data, '\n', count) != nullptr)
		__libc_file_flush(stream);

	return done;
}

/*
 * C23 7.24.4.4p4: exit flushes and closes the streams after the atexit
 * handlers; exit runs .fini_array then. Buffered output can only exist when
 * this object is linked, so the destructor lives here.
 */
[[gnu::destructor]] static void flush_at_exit(void)
{
	__libc_stdio_exit();
}
