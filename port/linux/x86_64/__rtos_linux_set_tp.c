/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_set_tp.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "linux.h"

/* The x86_64 thread pointer is the %fs segment base */
void __rtos_linux_set_tp(void *tp)
{
	__linux_syscall2(__linux_sys_arch_prctl, __linux_arch_set_fs, (long)tp);
}
