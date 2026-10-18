/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rtos-none.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _RTOS_NONE_H
#define _RTOS_NONE_H

#include <stddef.h>

/*
 * Bare-metal, single-threaded port. The standard output and error streams go
 * to __rtos_none_output, a weak default that discards the bytes; a board
 * replaces it (e.g. with a UART driver) by defining its own.
 */
void __rtos_none_output(const void *buffer, size_t count);

#endif
