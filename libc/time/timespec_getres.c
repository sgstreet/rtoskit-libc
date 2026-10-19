/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * timespec_getres.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <time.h>

/* C23 7.29.2.7: base on success, 0 on failure; ts may be null */
int timespec_getres(struct timespec *ts, int base)
{
	struct timespec resolution;
	int status = __rtos_clock_getres(base, &resolution);

	if (status < 0) {
		errno = -status;
		return 0;
	}
	if (ts != nullptr)
		*ts = resolution;

	return base;
}
