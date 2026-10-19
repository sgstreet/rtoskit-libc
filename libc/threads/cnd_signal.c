/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cnd_signal.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <threads.h>

/* C23 7.28.3.4: a new sequence number, and one waiter woken */
int cnd_signal(cnd_t *cond)
{
	__atomic_fetch_add(&cond->__sequence, 1, __ATOMIC_SEQ_CST);

	return __rtos_futex_wake(&cond->__sequence, 1) < 0 ? thrd_error : thrd_success;
}
