/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_before_input.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/*
 * Flushing earlier than the standard requires is harmless, so this runs
 * before every input operation on such a stream rather than only when the
 * buffer is empty, which would need the stream's lock held across it.
 */
void __libc_file_before_input(FILE *stream)
{
	if (stream->flags & (__LIBC_FILE_LINE | __LIBC_FILE_UNBUFFERED))
		__libc_stdio_flush_all(true);
}
