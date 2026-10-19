/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setvbuf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include "internal/file.h"

/*
 * C23 7.23.5.6: only before the first operation on the stream. A null buf
 * with a size sets the size of the buffer allocated at the first transfer.
 */
int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size)
{
	int status = 0;

	if (mode != _IOFBF && mode != _IOLBF && mode != _IONBF)
		return EOF;

	__libc_file_lock(stream);
	if (stream->flags & (__LIBC_FILE_READING | __LIBC_FILE_WRITING | __LIBC_FILE_UNGOT)) {
		status = EOF;
	} else {
		if (stream->flags & __LIBC_FILE_OWN_BUFFER)
			free(stream->buffer);
		stream->flags &= ~(__LIBC_FILE_LINE | __LIBC_FILE_UNBUFFERED | __LIBC_FILE_OWN_BUFFER);
		stream->buffer = nullptr;
		stream->size = 0;

		if (mode == _IONBF) {
			stream->flags |= __LIBC_FILE_UNBUFFERED;
		} else {
			if (mode == _IOLBF)
				stream->flags |= __LIBC_FILE_LINE;
			stream->size = size;
			if (buf != nullptr && size > 0) {
				stream->buffer = (unsigned char *)buf;
				stream->pos = stream->buffer;
				stream->end = stream->buffer;
			}
		}
	}
	__libc_file_unlock(stream);

	return status;
}
