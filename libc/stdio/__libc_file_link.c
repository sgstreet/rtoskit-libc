/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_link.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/file.h"

void __libc_file_link(FILE *stream)
{
	__rtos_lock_acquire(&__rtos_stdio_lock);
	stream->next = __libc_files;
	__libc_files = stream;
	__rtos_lock_release(&__rtos_stdio_lock);
}
