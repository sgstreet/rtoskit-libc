/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_futex_wake.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <rtos/rtos-port.h>

#include "linux.h"

int __rtos_futex_wake(unsigned int *address, unsigned int count)
{
	return (int)__linux_syscall3(__LINUX_SYS_FUTEX, (long)address, __LINUX_FUTEX_WAKE | __LINUX_FUTEX_PRIVATE, count > INT_MAX ? INT_MAX : (long)count);
}
