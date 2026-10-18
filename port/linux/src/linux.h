/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * linux.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LINUX_H
#define _LINUX_H

#include "linux-arch.h"

#define __LINUX_PT_TLS 7

#define __LINUX_PROT_READ 0x1
#define __LINUX_PROT_WRITE 0x2
#define __LINUX_MAP_PRIVATE 0x02
#define __LINUX_MAP_ANONYMOUS 0x20

#define __LINUX_SIGABRT 6

/* Kernel results in this range are negated error numbers */
static inline bool __linux_is_error(long result)
{
	return result < 0 && result > -4096;
}

#include <rtos/rtos-port.h>

/* The environment passed by the kernel, kept for __rtos_getenv */
extern char **__rtos_linux_environ;

/* The executable's TLS image, needed again for every new thread */
extern struct __libc_tls_image __rtos_linux_tls_image;

[[noreturn]] void __rtos_linux_start(long *stack);
void __rtos_linux_set_tp(void *tp);
void *__rtos_linux_get_tp(void);

#endif
