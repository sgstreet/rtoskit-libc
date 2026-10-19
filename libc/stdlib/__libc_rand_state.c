/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_rand_state.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/stdlib.h"

/* C23 7.24.2.2p2: without srand the sequence is that of srand(1); this is the state srand(1) produces */
thread_local uint64_t __libc_rand_state = (__LIBC_RAND_INCREMENT + 1) * __LIBC_RAND_MULTIPLIER + __LIBC_RAND_INCREMENT;
