/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exit_run.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/stdlib.h"

/*
 * Call the handlers newest first (C23 7.24.4.4p3). One is taken off the table
 * at a time, without the lock held during the call, so a handler may register
 * another, which then runs next.
 */
void __libc_exit_run(struct __libc_exit_handlers *table)
{
	while (true) {
		void (*handler)(void) = nullptr;

		__rtos_lock_acquire(&__rtos_atexit_lock);
		if (table->count > 0)
			handler = table->handlers[--table->count];
		__rtos_lock_release(&__rtos_atexit_lock);

		if (handler == nullptr)
			return;
		handler();
	}
}
