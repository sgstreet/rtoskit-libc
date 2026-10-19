/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * vfprintf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stdio.h>

#include "internal/file.h"
#include "internal/format.h"

struct stream_sink {
	struct __libc_format_sink sink;
	FILE *stream;
};

static void stream_write(struct __libc_format_sink *sink, const char *s, size_t n)
{
	struct stream_sink *target = (struct stream_sink *)sink;

	__libc_file_write(target->stream, s, n);
}

/* C23 7.23.6.8: the whole call holds the stream's lock; a write error makes the result negative */
int vfprintf(FILE *restrict stream, const char *restrict format, va_list arg)
{
	struct stream_sink target = { .sink = { .write = stream_write }, .stream = stream };
	int status;

	__libc_file_lock(stream);

	unsigned short error = stream->flags & __LIBC_FILE_ERROR;

	stream->flags &= ~__LIBC_FILE_ERROR;
	status = __libc_format(&target.sink, format, arg);
	if (stream->flags & __LIBC_FILE_ERROR)
		status = -1;
	stream->flags |= error;
	__libc_file_unlock(stream);

	return status;
}
