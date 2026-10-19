/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * signal.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <signal.h>

#include "internal/signal.h"

/* C23 7.14.1.1: returns the previous handler, or SIG_ERR for an invalid request */
void (*signal(int sig, void (*func)(int)))(int)
{
	void (*previous)(int);

	if (!__libc_signal_valid(sig) || func == SIG_ERR) {
		errno = EINVAL;
		return SIG_ERR;
	}

	previous = __libc_signal_handlers[sig];
	__libc_signal_handlers[sig] = func;

	return previous;
}
