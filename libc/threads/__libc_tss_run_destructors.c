/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tss_run_destructors.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* The key table, and each thread's values (PLAN D6: linked only with tss_create) */
struct __libc_tss_key __libc_tss_keys[__LIBC_TSS_MAX];
thread_local struct __libc_tss_value __libc_tss_values[__LIBC_TSS_MAX];

/*
 * C23 7.28.5.5p2-3: each non-null value of a key with a destructor is set to
 * null and passed to the destructor, in passes, while any destructor ran,
 * at most TSS_DTOR_ITERATIONS passes.
 */
void __libc_tss_run_destructors(void)
{
	for (int pass = 0; pass < TSS_DTOR_ITERATIONS; ++pass) {
		bool called = false;

		for (unsigned int key = 0; key < __LIBC_TSS_MAX; ++key) {
			unsigned int generation = __atomic_load_n(&__libc_tss_keys[key].generation, __ATOMIC_ACQUIRE);
			tss_dtor_t dtor = __atomic_load_n(&__libc_tss_keys[key].dtor, __ATOMIC_RELAXED);
			void *value = __libc_tss_values[key].value;

			if ((generation & 1) != 0 && dtor != nullptr && value != nullptr && __libc_tss_values[key].generation == generation) {
				__libc_tss_values[key].value = nullptr;
				dtor(value);
				called = true;
			}
		}

		if (!called)
			return;
	}
}
