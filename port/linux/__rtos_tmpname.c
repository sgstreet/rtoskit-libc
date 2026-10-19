/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_tmpname.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>

#include "linux.h"

/* Appends the decimal digits of value at p, returns the end */
static char *append_number(char *p, unsigned long value)
{
	char digits[20];
	int n = 0;

	do {
		digits[n++] = '0' + value % 10;
		value /= 10;
	} while (value != 0);
	while (n > 0)
		*p++ = digits[--n];

	return p;
}

/* "/tmp/rtoskit-<pid>-<index>": the process ID keeps concurrent test runs apart */
int __rtos_tmpname(char *buffer, size_t size, unsigned int index)
{
	static const char prefix[] = "/tmp/rtoskit-";
	char name[sizeof(prefix) + 2 * 20 + 2];
	char *p = name;

	for (const char *s = prefix; *s != '\0'; ++s)
		*p++ = *s;
	p = append_number(p, (unsigned long)__linux_syscall0(__LINUX_SYS_GETPID));
	*p++ = '-';
	p = append_number(p, index);
	*p++ = '\0';

	if ((size_t)(p - name) > size)
		return -ENAMETOOLONG;
	for (size_t i = 0; i < (size_t)(p - name); ++i)
		buffer[i] = name[i];

	return 0;
}
