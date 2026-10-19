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
#include <time.h>

#include "rtos-none.h"

/* Busy-waits on the monotonic clock, if the board provides one */
int __rtos_thread_sleep(const struct timespec *duration, struct timespec *)
{
	struct timespec start;
	struct timespec now;
	int status;

	if (duration->tv_sec < 0 || duration->tv_nsec < 0 || duration->tv_nsec >= 1000000000)
		return -EINVAL;
	status = __rtos_clock_gettime(TIME_MONOTONIC, &start);
	if (status < 0)
		return status;

	do {
		status = __rtos_clock_gettime(TIME_MONOTONIC, &now);
		if (status < 0)
			return status;
	} while ((now.tv_sec - start.tv_sec) * 1000000000LL + (now.tv_nsec - start.tv_nsec) < duration->tv_sec * 1000000000LL + duration->tv_nsec);

	return 0;
}
