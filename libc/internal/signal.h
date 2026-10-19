/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * signal.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_SIGNAL_H
#define _INTERNAL_SIGNAL_H

#include <signal.h>

/* Handlers indexed by signal number; signals 1 to __LIBC_SIGNAL_COUNT - 1 are valid */
#define __LIBC_SIGNAL_COUNT 32

extern void (*__libc_signal_handlers[__LIBC_SIGNAL_COUNT])(int);

static inline bool __libc_signal_valid(int sig)
{
	return sig > 0 && sig < __LIBC_SIGNAL_COUNT;
}

#endif /* _INTERNAL_SIGNAL_H */
