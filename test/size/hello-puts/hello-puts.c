/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hello-puts.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

/* Reference program: the smallest use of stdio */
int main(void)
{
	return puts("hello, world") < 0;
}
