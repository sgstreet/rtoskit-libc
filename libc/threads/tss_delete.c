/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tss_delete.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.6.2: no destructors run; the next generation is even (free) and invalidates every value */
void tss_delete(tss_t key)
{
	if (key >= __LIBC_TSS_MAX)
		return;

	__atomic_store_n(&__libc_tss_keys[key].dtor, nullptr, __ATOMIC_RELAXED);
	__atomic_fetch_add(&__libc_tss_keys[key].generation, 1, __ATOMIC_ACQ_REL);
}
