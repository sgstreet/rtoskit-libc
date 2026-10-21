/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * picolibc.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "linux-call.h"

/*
 * The OS layer a benchmark program needs from picolibc under qemu-arm:
 * tinystdio's standard streams writing through Linux write(2), the POSIX
 * calls behind fopen (the stdio benchmark writes to /dev/null), exit, and
 * the heap behind its sbrk. bench_cycles is a stub: the comparison counts
 * instructions only.
 */
/* From picolibc.ld; picolibc's arm TLS pointer */
extern char __bss_start[];
extern char __bss_end[];
extern char __tls_base[];
void _set_tls(void *tls);

/*
 * What picolibc's crt0 does that the loader does not: .bss (with .tbss) is
 * zeroed here, since its first page shares a page of the file with .data,
 * and the TLS pointer is set for errno
 */
void shim_init(void)
{
	memset(__bss_start, 0, (size_t)(__bss_end - __bss_start));
	_set_tls(__tls_base);
}

static int put(char c, FILE *file)
{
	linux_call(LINUX_WRITE, 1, (long)&c, 1);
	return (unsigned char)c;
}

static FILE streams = FDEV_SETUP_STREAM(put, nullptr, nullptr, _FDEV_SETUP_WRITE);

FILE *const stdin = &streams;
FILE *const stdout = &streams;
FILE *const stderr = &streams;

static char heap[HEAP_SIZE] __attribute__((aligned(8)));
static size_t used;

int open(const char *path, int flags, ...)
{
	return (int)linux_call(LINUX_OPEN, (long)path, flags, 0666);
}

int close(int fd)
{
	return (int)linux_call(LINUX_CLOSE, fd, 0, 0);
}

ssize_t read(int fd, void *buffer, size_t length)
{
	return linux_call(LINUX_READ, fd, (long)buffer, (long)length);
}

ssize_t write(int fd, const void *buffer, size_t length)
{
	return linux_call(LINUX_WRITE, fd, (long)buffer, (long)length);
}

off_t lseek(int fd, off_t offset, int whence)
{
	return linux_call(LINUX_LSEEK, fd, offset, whence);
}

void _exit(int status)
{
	while (true)
		linux_call(LINUX_EXIT_GROUP, status, 0, 0);
}

void *sbrk(ptrdiff_t increment)
{
	void *block = heap + used;

	if (increment < 0 || used + (size_t)increment > sizeof(heap))
		return (void *)-1;

	used += (size_t)increment;
	return block;
}

unsigned long long bench_cycles(void)
{
	return 0;
}
