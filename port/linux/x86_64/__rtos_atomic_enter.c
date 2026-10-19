/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_atomic_enter.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

unsigned int __rtos_linux_atomic_lock;

/* A spin lock that yields while another thread holds it; x86_64 exchanges inline */
unsigned long __rtos_atomic_enter(void)
{
	while (__atomic_exchange_n(&__rtos_linux_atomic_lock, 1, __ATOMIC_ACQUIRE) != 0)
		__linux_syscall0(__LINUX_SYS_SCHED_YIELD);

	return 0;
}
