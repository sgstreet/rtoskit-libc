/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * linux.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LINUX_H
#define _LINUX_H

#include <time.h>

#include "linux-arch.h"

#define __LINUX_PT_TLS 7

#define __LINUX_PROT_READ 0x1
#define __LINUX_PROT_WRITE 0x2
#define __LINUX_MAP_PRIVATE 0x02
#define __LINUX_MAP_ANONYMOUS 0x20
#define __LINUX_MAP_NORESERVE 0x4000

#define __LINUX_SIGABRT 6

/* openat flags and arguments; the values are the same on every architecture except O_LARGEFILE */
#define __LINUX_O_RDONLY 0
#define __LINUX_O_WRONLY 1
#define __LINUX_O_RDWR 2
#define __LINUX_O_CREAT 0x40
#define __LINUX_O_EXCL 0x80
#define __LINUX_O_TRUNC 0x200
#define __LINUX_O_APPEND 0x400
#define __LINUX_O_CLOEXEC 0x80000
#define __LINUX_AT_FDCWD -100
#define __LINUX_CREATE_MODE 0666

/* Clock IDs for the time bases, and the kernel's 64-bit timespec */
#define __LINUX_CLOCK_REALTIME 0
#define __LINUX_CLOCK_MONOTONIC 1
#define __LINUX_CLOCK_PROCESS_CPUTIME_ID 2
#define __LINUX_CLOCK_THREAD_CPUTIME_ID 3

struct __linux_timespec {
	long long tv_sec;
	long long tv_nsec;
};

/* The kernel clock for a TIME_* base, or -1 */
static inline int __linux_clock(int base)
{
	switch (base) {
		case TIME_UTC:
			return __LINUX_CLOCK_REALTIME;
		case TIME_MONOTONIC:
			return __LINUX_CLOCK_MONOTONIC;
		case TIME_ACTIVE:
			return __LINUX_CLOCK_PROCESS_CPUTIME_ID;
		case TIME_THREAD_ACTIVE:
			return __LINUX_CLOCK_THREAD_CPUTIME_ID;
		default:
			return -1;
	}
}

/* The heap: reserved address space, committed by the kernel page by page as it is touched */
#define __LINUX_HEAP_SIZE (64UL << 20)

/* Locks the port can create at run time (until thread support, P1.12) */
#define __LINUX_LOCK_POOL_SIZE 32

/*
 * Until thread support (P1.12) the program is single-threaded, so a lock only
 * tracks its recursion depth; releasing a lock that is not held aborts, which
 * makes unbalanced locking visible in the tests.
 */
struct __rtos_lock {
	unsigned int depth;
	bool allocated;
};

/* Kernel results in this range are negated error numbers */
static inline bool __linux_is_error(long result)
{
	return result < 0 && result > -4096;
}

#include "rtos-linux.h"

/* Defined by the linker when the ELF header is part of a loaded segment */
extern const struct __linux_ehdr __ehdr_start;

/* The program's entry point, called through __libc_start_main */
extern int main(int argc, char **argv);

[[noreturn]] void __rtos_linux_start(long *stack);

#endif /* _LINUX_H */
