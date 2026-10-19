/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fseek.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/*
 * C23 7.23.9.2: pending output is written, unread input and pushback are
 * dropped, and the end-of-file indicator is cleared. SEEK_CUR is relative to
 * the position the stream shows, not the handle's.
 */
int fseek(FILE *stream, long int offset, int whence)
{
	long long target = offset;
	int status = 0;

	if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		errno = EINVAL;
		return -1;
	}

	__libc_file_lock(stream);
	if (whence == SEEK_CUR) {
		long long position = __libc_file_position(stream);

		if (position < 0)
			status = -1;
		target += position;
		whence = SEEK_SET;
	}
	if (status == 0 && (stream->flags & __LIBC_FILE_WRITING) && __libc_file_flush(stream) != 0)
		status = -1;
	if (status == 0) {
		stream->flags &= ~(__LIBC_FILE_READING | __LIBC_FILE_WRITING | __LIBC_FILE_UNGOT | __LIBC_FILE_EOF);
		stream->pos = stream->buffer;
		stream->end = stream->buffer;

		long long result = __rtos_seek(stream->handle, target, whence);

		if (result < 0) {
			errno = (int)-result;
			status = -1;
		}
	}
	__libc_file_unlock(stream);

	return status;
}
