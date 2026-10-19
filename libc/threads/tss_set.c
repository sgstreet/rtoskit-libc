/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tss_set.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.6.4 */
int tss_set(tss_t key, void *val)
{
	unsigned int generation;

	if (key >= __LIBC_TSS_MAX)
		return thrd_error;
	generation = __atomic_load_n(&__libc_tss_keys[key].generation, __ATOMIC_ACQUIRE);
	if ((generation & 1) == 0)
		return thrd_error;

	__libc_tss_values[key] = (struct __libc_tss_value){ .value = val, .generation = generation };

	return thrd_success;
}
