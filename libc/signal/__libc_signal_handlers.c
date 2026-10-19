/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_signal_handlers.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/signal.h"

/* Every signal starts with SIG_DFL (C23 7.14.1.1p6), which is the zero pointer */
void (*__libc_signal_handlers[__LIBC_SIGNAL_COUNT])(int);
