/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fsetpos.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.9.3: as fseek to the stored position, which may exceed long */
int fsetpos(FILE *stream, const fpos_t *pos)
{
	int status = 0;

	__libc_file_lock(stream);
	if ((stream->flags & __LIBC_FILE_WRITING) && __libc_file_flush(stream) != 0)
		status = -1;
	if (status == 0) {
		stream->flags &= ~(__LIBC_FILE_READING | __LIBC_FILE_WRITING | __LIBC_FILE_UNGOT | __LIBC_FILE_EOF);
		stream->pos = stream->buffer;
		stream->end = stream->buffer;

		long long result = __rtos_seek(stream->handle, pos->__offset, SEEK_SET);

		if (result < 0) {
			errno = (int)-result;
			status = -1;
		}
	}
	__libc_file_unlock(stream);

	return status;
}
