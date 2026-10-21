/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * newlib.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>

#include "linux-call.h"

/*
 * The system calls a benchmark program needs from newlib-nano under
 * qemu-arm: open, close, read, write and lseek (the stdio benchmark writes
 * to /dev/null), exit, the heap behind sbrk, and the standard output
 * reported as a terminal; libnosys supplies the rest. bench_cycles is a
 * stub: the comparison counts instructions only.
 */
static char heap[HEAP_SIZE] __attribute__((aligned(8)));
static size_t used;

/* newlib-nano needs no more than the loader does: .data and .bss share one segment */
void shim_init(void)
{
}

int _open(const char *path, int flags, int mode)
{
	return (int)linux_call(LINUX_OPEN, (long)path, flags, mode);
}

int _close(int fd)
{
	return (int)linux_call(LINUX_CLOSE, fd, 0, 0);
}

int _read(int fd, void *buffer, size_t length)
{
	return (int)linux_call(LINUX_READ, fd, (long)buffer, (long)length);
}

int _lseek(int fd, int offset, int whence)
{
	return (int)linux_call(LINUX_LSEEK, fd, offset, whence);
}

int _write(int fd, const void *buffer, size_t length)
{
	return (int)linux_call(LINUX_WRITE, fd, (long)buffer, (long)length);
}

void _exit(int status)
{
	while (true)
		linux_call(LINUX_EXIT_GROUP, status, 0, 0);
}

void *_sbrk(ptrdiff_t increment)
{
	void *block = heap + used;

	if (increment < 0 || used + (size_t)increment > sizeof(heap)) {
		errno = ENOMEM;
		return (void *)-1;
	}

	used += (size_t)increment;
	return block;
}

int _fstat(int fd, struct stat *status)
{
	status->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int fd)
{
	return 1;
}

unsigned long long bench_cycles(void)
{
	return 0;
}
