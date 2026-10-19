/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * thrd_exit.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

/*
 * C23 7.28.5.5p5: when the initial thread calls thrd_exit, the program goes
 * on until the last thread ends, then terminates as if by
 * exit(EXIT_SUCCESS): atexit handlers run and buffered output is flushed.
 * thrd_exit.run checks the output and the status.
 */

static void at_exit(void)
{
	puts("atexit handler ran");
}

static int worker(void *)
{
	thrd_sleep(&(struct timespec){ .tv_nsec = 50000000 }, nullptr);
	/* Still buffered when the thread ends; exit must flush it */
	fputs("worker done\n", stdout);
	return 7;
}

int main(void)
{
	thrd_t thread;

	if (atexit(at_exit) != 0 || thrd_create(&thread, worker, nullptr) != thrd_success)
		return 1;
	puts("main exiting");
	thrd_exit(3);
}
