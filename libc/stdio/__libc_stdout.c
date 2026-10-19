/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_stdout.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/file.h"

/* Line buffered: whether it is interactive is unknown, and C23 7.23.3p7 allows full buffering only when it can be determined not to be */
FILE __libc_stdout = { .handle = 1, .flags = __LIBC_FILE_WRITE | __LIBC_FILE_LINE, .lock = &__rtos_stdout_lock };
