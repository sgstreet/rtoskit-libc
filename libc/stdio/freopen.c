/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * freopen.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/*
 * C23 7.23.5.4. With a file name: close whatever the stream had (ignoring
 * failure) and open the file into the same FILE, which keeps its buffer and
 * lock. Without one, the implementation-defined mode changes allowed are
 * those that need no reopening: any access the stream already has.
 */
FILE *freopen(const char *restrict filename, const char *restrict mode, FILE *restrict stream)
{
	unsigned short keep = __LIBC_FILE_LINE | __LIBC_FILE_UNBUFFERED | __LIBC_FILE_OWN_BUFFER | __LIBC_FILE_USER_BUFFER | __LIBC_FILE_ALLOCATED;
	unsigned int open_flags;
	unsigned short stream_flags;
	int error = 0;

	__libc_file_lock(stream);
	__libc_file_flush(stream);

	if (!__libc_file_mode(mode, &open_flags, &stream_flags)) {
		error = EINVAL;
	} else if (filename == nullptr) {
		if ((stream_flags & (__LIBC_FILE_READ | __LIBC_FILE_WRITE) & ~stream->flags) != 0)
			error = EBADF;
	} else {
		__rtos_close(stream->handle);
		stream->handle = __rtos_open(filename, open_flags);
		if (stream->handle < 0)
			error = -stream->handle;
	}

	/* The new state clears the indicators and any pushback (7.23.5.4p4) */
	if (error == 0)
		stream->flags = (stream->flags & keep) | stream_flags;
	__libc_file_unlock(stream);

	if (error == 0)
		return stream;

	/* A failed freopen leaves the stream closed */
	fclose(stream);
	errno = error;
	return nullptr;
}
