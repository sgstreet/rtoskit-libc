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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <signal.h> (P1.05): handlers run once and are reset, SIG_IGN, invalid
 * signals; signal.run checks the default actions and abort with a handler.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static volatile sig_atomic_t received;

static void handler(int sig)
{
	received = sig;
}

static void returns_from_abort(int)
{
	puts("handler");
}

int main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : "";

	if (strcmp(mode, "term") == 0)
		raise(SIGTERM);
	if (strcmp(mode, "abort") == 0) {
		signal(SIGABRT, returns_from_abort);
		abort();
	}
	if (strcmp(mode, "ignored-abort") == 0) {
		signal(SIGABRT, SIG_IGN);
		abort();
	}

	CHECK(signal(SIGINT, handler) == SIG_DFL);
	CHECK(raise(SIGINT) == 0 && received == SIGINT);
	/* The handler was reset to SIG_DFL before it ran */
	CHECK(signal(SIGINT, SIG_IGN) == SIG_DFL);
	received = 0;
	CHECK(raise(SIGINT) == 0 && received == 0);
	CHECK(signal(SIGINT, SIG_DFL) == SIG_IGN);

	errno = 0;
	CHECK(signal(0, handler) == SIG_ERR && errno == EINVAL);
	errno = 0;
	CHECK(signal(1000, handler) == SIG_ERR && errno == EINVAL);
	CHECK(signal(SIGFPE, SIG_ERR) == SIG_ERR);
	CHECK(raise(-1) != 0 && raise(1000) != 0);

	if (failures == 0)
		puts("pass: signal");

	return failures == 0 ? 0 : 1;
}
