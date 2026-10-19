/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * abort.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <signal.h>
#include <stdlib.h>
#include <rtos/rtos-port.h>

/* C23 7.24.4.1: abnormal termination by raise(SIGABRT), even when a handler returns */
void abort(void)
{
	raise(SIGABRT);
	__rtos_abort();
}
