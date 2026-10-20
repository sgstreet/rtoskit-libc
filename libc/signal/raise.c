/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * raise.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <signal.h>
#include <stdlib.h>
#include <rtos/rtos-port.h>

#include "internal/signal.h"

/*
 * The handlers, indexed by signal number. They live with raise, so signal
 * links raise, and abort can reference raise weakly (PLAN D52). Every signal
 * starts with SIG_DFL (C23 7.14.1.1p6), which is the zero pointer.
 */
void (*__libc_signal_handlers[__LIBC_SIGNAL_COUNT])(int);

/*
 * C23 7.14.2.1. A handler is reset to SIG_DFL before it runs (7.14.1.1p3
 * allows that). The default action for SIGABRT is abnormal termination;
 * for the other signals the program ends with status 128 + sig, as a shell
 * reports a signal (implementation-defined).
 */
int raise(int sig)
{
	void (*handler)(int);

	if (!__libc_signal_valid(sig))
		return -1;

	handler = __libc_signal_handlers[sig];
	if (handler == SIG_IGN)
		return 0;

	if (handler == SIG_DFL) {
		if (sig == SIGABRT)
			__rtos_abort();
		_Exit(128 + sig);
	}

	__libc_signal_handlers[sig] = SIG_DFL;
	handler(sig);

	return 0;
}
