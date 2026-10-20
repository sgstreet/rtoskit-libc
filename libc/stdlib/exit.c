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
 * destructors in reverse order; stdio flushes and closes the streams from
 * there. The handlers run only when atexit is linked (PLAN D52).
 */
void exit(int status)
{
	if (__libc_atexit_run != nullptr)
		__libc_atexit_run();

	for (void (*const *entry)(void) = __fini_array_end; entry > __fini_array_start; --entry)
		entry[-1]();

	__rtos_exit(status);
}
