/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_thread_sleep.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

/* clock_nanosleep on CLOCK_REALTIME: C measures the sleep by TIME_UTC (7.28.5.7p3) */
int __rtos_thread_sleep(const struct timespec *duration, struct timespec *remaining)
{
	struct __linux_timespec request = { .tv_sec = duration->tv_sec, .tv_nsec = duration->tv_nsec };
	struct __linux_timespec left = { 0 };
	long status;

	if (duration->tv_sec < 0 || duration->tv_nsec < 0 || duration->tv_nsec >= 1000000000)
		return -EINVAL;

	status = __linux_syscall6(__LINUX_SYS_CLOCK_NANOSLEEP, __LINUX_CLOCK_REALTIME, 0, (long)&request, (long)&left, 0, 0);
	if (status == -EINTR && remaining != nullptr) {
		remaining->tv_sec = left.tv_sec;
		remaining->tv_nsec = (long)left.tv_nsec;
	}

	return (int)status;
}
