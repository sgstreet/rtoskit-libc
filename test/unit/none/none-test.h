/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * none-test.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _NONE_TEST_H
#define _NONE_TEST_H

#include <stddef.h>

#include "linux-arch.h"

/*
 * The "board" of test/unit/none: a Linux process. Start-up and two system
 * calls are written per architecture family (none-x86_64.S, none-arm.S);
 * on x86_64 that file also replaces __rtos_none_set_tp.
 */
void none_test_write(const void *buffer, size_t count);
[[noreturn]] void none_test_exit(int status);

/* The ELF header, mapped with the program; its program headers locate the TLS image */
extern const struct __linux_ehdr __ehdr_start;

/* Called by _start with the initial stack */
[[noreturn]] void none_test_start(long *stack);

#endif /* _NONE_TEST_H */
