/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_stdio_exit.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/* The standard streams are flushed but stay open; the others are closed, which also removes tmpfile files */
void __libc_stdio_exit(void)
{
	__libc_stdio_flush_all(false);

	while (__libc_files != nullptr)
		fclose(__libc_files);
}
