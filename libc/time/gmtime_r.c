/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * gmtime_r.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/* C23 7.29.3.4: null with errno EOVERFLOW when the year does not fit */
struct tm *gmtime_r(const time_t *timer, struct tm *buf)
{
	if (!__libc_secs_to_tm(*timer, buf))
		return nullptr;

	buf->tm_isdst = 0;
	return buf;
}
