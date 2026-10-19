/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <rtos/rtos-port.h>

#include "internal/sections.h"
#include "internal/stdlib.h"

/*
 * C23 7.24.4.4: the atexit handlers, newest first, then the static
 * destructors in reverse order. Stream flushing joins with P1.09.
 */
void exit(int status)
{
	__libc_exit_run(&__libc_atexit_handlers);

	for (void (*const *entry)(void) = __fini_array_end; entry > __fini_array_start; --entry)
		entry[-1]();

	__rtos_exit(status);
}
