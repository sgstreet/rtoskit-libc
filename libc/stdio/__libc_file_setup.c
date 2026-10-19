/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_setup.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include "internal/file.h"

/*
 * The buffer comes at the first transfer, so an unused stream costs none;
 * size is BUFSIZ unless setvbuf chose one. Without memory the stream works
 * unbuffered rather than failing.
 */
void __libc_file_setup(FILE *stream)
{
	if (stream->buffer != nullptr)
		return;

	if (!(stream->flags & __LIBC_FILE_UNBUFFERED)) {
		size_t size = stream->size != 0 ? stream->size : BUFSIZ;

		stream->buffer = malloc(size);
		if (stream->buffer != nullptr) {
			stream->size = size;
			stream->flags |= __LIBC_FILE_OWN_BUFFER;
		}
	}
	if (stream->buffer == nullptr) {
		stream->flags |= __LIBC_FILE_UNBUFFERED;
		stream->buffer = &stream->single;
		stream->size = 1;
	}

	stream->pos = stream->buffer;
	stream->end = stream->buffer;
}
