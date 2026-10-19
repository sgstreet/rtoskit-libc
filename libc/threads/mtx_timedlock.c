/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * mtx_timedlock.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.4.4: ts is an absolute TIME_UTC time, which the seam's wait takes as is */
int mtx_timedlock(mtx_t *restrict mtx, const struct timespec *restrict ts)
{
	if ((mtx->__type & mtx_timed) == 0)
		return thrd_error;

	return __libc_mutex_lock(mtx, ts);
}
