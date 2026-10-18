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
	__linux_syscall3(__linux_sys_tgkill, __linux_syscall0(__linux_sys_getpid), __linux_syscall0(__linux_sys_gettid), __linux_sigabrt);

	for (;;)
		__linux_syscall1(__linux_sys_exit_group, 128 + __linux_sigabrt);
}
