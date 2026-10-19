/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tss_create.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.6.1: claim a free key (even generation) by making its generation odd */
int tss_create(tss_t *key, tss_dtor_t dtor)
{
	for (unsigned int index = 0; index < __LIBC_TSS_MAX; ++index) {
		unsigned int generation = __atomic_load_n(&__libc_tss_keys[index].generation, __ATOMIC_RELAXED);

		if ((generation & 1) == 0 && __atomic_compare_exchange_n(&__libc_tss_keys[index].generation, &generation, generation + 1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
			__atomic_store_n(&__libc_tss_keys[index].dtor, dtor, __ATOMIC_RELEASE);
			*key = index;
			return thrd_success;
		}
	}

	return thrd_error;
}
