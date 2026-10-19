/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_stderr.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/file.h"

/* Not fully buffered (C23 7.23.3p7); unbuffered, so messages are never held back */
FILE __libc_stderr = { .handle = 2, .flags = __LIBC_FILE_WRITE | __LIBC_FILE_UNBUFFERED, .lock = &__rtos_stderr_lock };
