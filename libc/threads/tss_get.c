/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tss_get.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <threads.h>

#include "internal/threads.h"

/* C23 7.28.6.3: a value set under an older generation of the key is null */
void *tss_get(tss_t key)
{
	if (key >= __LIBC_TSS_MAX || __libc_tss_values[key].generation != __atomic_load_n(&__libc_tss_keys[key].generation, __ATOMIC_ACQUIRE))
		return nullptr;

	return __libc_tss_values[key].value;
}
