/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_read.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <stdio.h>
#include <string.h>

#include "internal/file.h"

size_t __libc_file_read(FILE *stream, void *buffer, size_t count)
{
	unsigned char *data = buffer;
	size_t done = 0;

	while (done < count) {
		/* The pushed-back character, then the window, then the handle */
		if ((stream->flags & (__LIBC_FILE_UNGOT | __LIBC_FILE_READING)) == __LIBC_FILE_READING && stream->pos < stream->end) {
			size_t chunk = stream->end - stream->pos;

			if (chunk > count - done)
				chunk = count - done;
			memcpy(data + done, stream->pos, chunk);
			stream->pos += chunk;
			done += chunk;
			continue;
		}

		int c = __libc_file_getc(stream);

		if (c == EOF)
			break;
		data[done++] = c;
	}

	return done;
}
