/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_abort.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* Die by SIGABRT so the parent sees an abnormal termination; exit if the signal is ignored */
void __rtos_abort(void)
{
	__linux_syscall3(__LINUX_SYS_TGKILL, __linux_syscall0(__LINUX_SYS_GETPID), __linux_syscall0(__LINUX_SYS_GETTID), __LINUX_SIGABRT);

	while (true)
		__linux_syscall1(__LINUX_SYS_EXIT_GROUP, 128 + __LINUX_SIGABRT);
}
