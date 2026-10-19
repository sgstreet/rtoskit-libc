/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * rtos-port.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _RTOS_RTOS_PORT_H
#define _RTOS_RTOS_PORT_H

/*
 * The RTOS seam (PLAN D4; process and I/O from P1.01, heap and locks from P1.04).
 *
 * The library reaches the operating system only through the __rtos_*
 * functions below. Exactly one port library (librtos-<port>.a) defines them;
 * they are bound at link time. The __libc_* functions are the services libc
 * provides to a port.
 *
 * Error convention: functions returning ptrdiff_t or int report failure as a
 * negated <errno.h> value, which libc stores in errno.
 */

#include <stddef.h>
#include <stdint.h>

/* Process control */

/* Terminate the program normally with the given status; never returns */
[[noreturn]] void __rtos_exit(int status);

/* Terminate the program abnormally (abort); never returns */
[[noreturn]] void __rtos_abort(void);

/* I/O. Handles 0, 1 and 2 are the standard input, output and error streams */

/* Read up to count bytes; returns the number read, 0 at end of file, or a negated errno */
ptrdiff_t __rtos_read(int handle, void *buffer, size_t count);

/* Write up to count bytes; returns the number written or a negated errno */
ptrdiff_t __rtos_write(int handle, const void *buffer, size_t count);

/*
 * Heap (PLAN D31). libc's allocator asks for its memory once, at the first
 * allocation, by calling this with index 0, 1, 2, ... until it returns false
 * (at most 8 regions). Each region is a fixed block of memory the allocator
 * owns from then on; it needs no particular alignment. A port without heap
 * memory returns false for index 0, and every allocation then fails.
 */
bool __rtos_heap_region(unsigned int index, void **base, size_t *size);

/*
 * Locks (PLAN D31). The port defines struct __rtos_lock; libc only handles
 * pointers to it, so one libc.a serves every port. Locks are recursive: the
 * owning thread may acquire a lock it holds, and must release it as often.
 * libc names the statically allocated locks below, which the port defines;
 * __rtos_lock_create and __rtos_lock_destroy serve locks libc creates at run
 * time. A single-threaded port implements them all as no-ops.
 */
struct __rtos_lock;

/* Serialises the memory management functions */
extern struct __rtos_lock __rtos_malloc_lock;

void __rtos_lock_acquire(struct __rtos_lock *lock);
void __rtos_lock_release(struct __rtos_lock *lock);

/* Returns 0 and stores a new lock, or a negated errno */
int __rtos_lock_create(struct __rtos_lock **lock);
void __rtos_lock_destroy(struct __rtos_lock *lock);

/*
 * Thread-local storage (PLAN D10). A port describes the TLS initialization
 * image (the PT_TLS segment, or the equivalent linker symbols) and, for every
 * thread including the initial one, provides a block of
 * __libc_tls_block_size() bytes with no alignment requirement. libc lays out
 * the TLS data and the thread control block inside it for the arch's TLS
 * variant, and returns the value the port must install as the thread pointer.
 */
struct __libc_tls_image {
	const void *init; /* initialization image (.tdata); its address fixes the alignment phase */
	size_t init_size; /* bytes to copy from init */
	size_t size; /* total bytes (.tdata + .tbss); the rest is zero filled */
	size_t align; /* alignment of the TLS segment, a power of two (0 means 1) */
};

size_t __libc_tls_block_size(const struct __libc_tls_image *image);
void *__libc_tls_init(void *block, const struct __libc_tls_image *image);

/*
 * Program start. The port's startup code establishes the stack and the
 * initial thread's TLS, then calls this. It runs the static constructors,
 * calls main and passes its result to exit.
 */
[[noreturn]] void __libc_start_main(int (*main)(int, char **), int argc, char **argv);

#endif /* _RTOS_RTOS_PORT_H */
