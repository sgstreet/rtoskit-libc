/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fclose.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal/file.h"

/* C23 7.23.5.1: flush, close, release the buffer and the stream */
int fclose(FILE *stream)
{
	int status;
	int closed;

	/* Unlinked before its own lock is taken, keeping the list-then-stream lock order */
	__libc_file_unlink(stream);

	__libc_file_lock(stream);
	status = __libc_file_flush(stream);
	closed = __rtos_close(stream->handle);
	if (closed < 0) {
		errno = -closed;
		status = EOF;
	}
	if (stream->flags & __LIBC_FILE_OWN_BUFFER)
		free(stream->buffer);
	stream->buffer = nullptr;
	stream->size = 0;
	stream->handle = -1;
	stream->flags &= __LIBC_FILE_ALLOCATED;
	__libc_file_unlock(stream);

	if (stream->flags & __LIBC_FILE_ALLOCATED) {
		__rtos_lock_destroy(stream->lock);
		free(stream);
	}

	return status;
}
