/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * exit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Program termination (P1.05); exit.run checks the output and status of each
 * mode: atexit handlers newest first, a handler registering another one,
 * destructors after the handlers, quick_exit running only its own handlers,
 * and the 32-handler minimum.
 */

static void first(void)
{
	puts("first");
}

static void second(void)
{
	puts("second");
}

static void late(void)
{
	puts("late");
}

static void registers_late(void)
{
	puts("registers");
	atexit(late);
}

static void quick(void)
{
	puts("quick");
}

static void noop(void)
{
}

[[gnu::destructor]] static void destructor(void)
{
	puts("destructor");
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "";

	if (strcmp(mode, "atexit") == 0) {
		atexit(first);
		atexit(second);
		atexit(registers_late);
		at_quick_exit(quick);
		exit(5);
	}

	if (strcmp(mode, "quick") == 0) {
		atexit(first);
		at_quick_exit(quick);
		at_quick_exit(second);
		quick_exit(7);
	}

	if (strcmp(mode, "limit") == 0) {
		/* C23 7.24.4.2p3: at least 32 registrations succeed */
		for (int i = 0; i < 32; ++i)
			if (atexit(noop) != 0 || at_quick_exit(noop) != 0)
				return 1;
		puts(atexit(noop) != 0 ? "full" : "unlimited");
		return 0;
	}

	puts("usage: exit atexit|quick|limit");
	return 2;
}
