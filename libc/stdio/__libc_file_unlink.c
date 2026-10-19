/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_unlink.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/file.h"

/* Only allocated streams are linked; the standard streams are not */
void __libc_file_unlink(FILE *stream)
{
	if (!(stream->flags & __LIBC_FILE_ALLOCATED))
		return;

	__rtos_lock_acquire(&__rtos_stdio_lock);
	if (stream->prev != nullptr)
		stream->prev->next = stream->next;
	else
		__libc_files = stream->next;
	if (stream->next != nullptr)
		stream->next->prev = stream->prev;
	stream->next = nullptr;
	stream->prev = nullptr;
	__rtos_lock_release(&__rtos_stdio_lock);
}
