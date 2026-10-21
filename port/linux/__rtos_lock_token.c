/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_lock_token.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "linux.h"

/* The initial thread's token; every other thread sets its own when it starts (__rtos_thread_create.c) */
thread_local unsigned int __rtos_lock_token = __LINUX_MAIN_LOCK_TOKEN;
