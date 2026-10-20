/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hello-printf-int-minimal.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

/* Reference program: hello-printf-int linked against libc-minimal-printf.a (PLAN D53) */
int main(int argc, char **argv)
{
	unsigned int value = (unsigned int)argc * 2654435761u;

	return printf("%d %u %x %c %s\n", -argc, value, value, 'a' + argc, argv[0]) < 0;
}
