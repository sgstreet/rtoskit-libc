/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_new.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal/file.h"

/* An opened file is fully buffered: it is not taken to be an interactive device (C23 7.23.5.3p8) */
FILE *__libc_file_new(int handle, unsigned short flags)
{
	FILE *stream = calloc(1, sizeof(*stream));
	int status;

	if (stream == nullptr) {
		__rtos_close(handle);
		return nullptr;
	}

	status = __rtos_lock_create(&stream->lock);
	if (status < 0) {
		errno = -status;
		free(stream);
		__rtos_close(handle);
		return nullptr;
	}

	stream->handle = handle;
	stream->flags = flags | __LIBC_FILE_ALLOCATED;
	__libc_file_link(stream);

	return stream;
}
