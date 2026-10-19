/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ungetc.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.7.11: one character of pushback, which clears the end-of-file indicator */
int ungetc(int c, FILE *stream)
{
	int result = EOF;

	if (c == EOF)
		return EOF;

	__libc_file_lock(stream);
	if ((stream->flags & __LIBC_FILE_READ) && !(stream->flags & __LIBC_FILE_UNGOT)) {
		/* Pending output goes first: the pushback belongs to the input side */
		if (!(stream->flags & __LIBC_FILE_WRITING) || __libc_file_flush(stream) == 0) {
			stream->ungot = c;
			stream->flags |= __LIBC_FILE_UNGOT;
			stream->flags &= ~__LIBC_FILE_EOF;
			result = (unsigned char)c;
		}
	}
	__libc_file_unlock(stream);

	return result;
}
