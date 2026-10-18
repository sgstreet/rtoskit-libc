/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_write.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "internal/file.h"

/* Write all of buffer unless the port reports an error; returns the bytes written */
size_t __libc_file_write(FILE *stream, const void *buffer, size_t count)
{
	const unsigned char *next = buffer;
	size_t done = 0;

	while (done < count) {
		ptrdiff_t written = __rtos_write(stream->handle, next + done, count - done);

		if (written < 0) {
			errno = (int)-written;
			break;
		}
		done += (size_t)written;
	}

	return done;
}
