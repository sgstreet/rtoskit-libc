/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_stdio_flush_all.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/*
 * A standard stream the program never names holds no output: refer to them
 * weakly, so flushing links neither (PLAN D52).
 */
#pragma weak __libc_stdout
#pragma weak __libc_stderr

static int flush_one(FILE *stream, bool line_buffered_only)
{
	int status = 0;

	if (stream == nullptr)
		return 0;

	__libc_file_lock(stream);
	if ((stream->flags & __LIBC_FILE_WRITING) && (!line_buffered_only || (stream->flags & __LIBC_FILE_LINE)))
		status = __libc_file_flush(stream);
	__libc_file_unlock(stream);

	return status;
}

/*
 * Output streams only (C23 7.23.5.2p3 for fflush(NULL)). Lock order: the
 * list lock, then each stream's lock; nothing takes them the other way.
 */
int __libc_stdio_flush_all(bool line_buffered_only)
{
	int status = 0;

	status |= flush_one(stdout, line_buffered_only);
	status |= flush_one(stderr, line_buffered_only);

	__rtos_lock_acquire(&__rtos_stdio_lock);
	for (FILE *stream = __libc_files; stream != nullptr; stream = stream->next)
		status |= flush_one(stream, line_buffered_only);
	__rtos_lock_release(&__rtos_stdio_lock);

	return status != 0 ? EOF : 0;
}
