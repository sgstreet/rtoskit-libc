/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exit_register.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/stdlib.h"

/* Returns 0 on success and nonzero when the table is full (C23 7.24.4.2p4) */
int __libc_exit_register(struct __libc_exit_handlers *table, void (*func)(void))
{
	int status = -1;

	__rtos_lock_acquire(&__rtos_atexit_lock);
	if (table->count < __LIBC_EXIT_HANDLERS) {
		table->handlers[table->count++] = func;
		status = 0;
	}
	__rtos_lock_release(&__rtos_atexit_lock);

	return status;
}
