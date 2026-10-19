/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_clock_gettime.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <time.h>

#include "linux.h"

int __rtos_clock_gettime(int base, struct timespec *ts)
{
	struct __linux_timespec value;
	int clock = __linux_clock(base);

	if (clock < 0)
		return -EINVAL;

	long status = __linux_syscall2(__LINUX_SYS_CLOCK_GETTIME, clock, (long)&value);

	if (status < 0)
		return (int)status;

	ts->tv_sec = value.tv_sec;
	ts->tv_nsec = (long)value.tv_nsec;
	return 0;
}
