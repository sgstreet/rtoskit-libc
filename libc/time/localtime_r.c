/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * localtime_r.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <time.h>

#include "internal/time.h"

/* C23 7.29.3.6: TZ is read on every call, as if tzset had been called */
struct tm *localtime_r(const time_t *timer, struct tm *buf)
{
	struct __libc_tz tz;
	bool dst;

	__libc_tz_get(&tz);
	dst = __libc_tz_is_dst(&tz, *timer);

	if (!__libc_secs_to_tm(*timer + (dst ? tz.dst_offset : tz.std_offset), buf))
		return nullptr;

	buf->tm_isdst = dst;
	return buf;
}
