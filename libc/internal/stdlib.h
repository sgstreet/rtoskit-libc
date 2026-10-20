/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdlib.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_STDLIB_H
#define _INTERNAL_STDLIB_H

#include <stddef.h>
#include <stdint.h>

/*
 * The strtol family's core (C23 7.24.1.7): parse an integer into uintmax_t.
 * max is the largest positive value of the target type; signed targets also
 * accept magnitudes up to max + 1 when negative. The result is the value
 * (negated in two's complement when there was a minus sign) or, on overflow,
 * the clamped limit with errno = ERANGE.
 */
uintmax_t __libc_strtoint(const char *restrict nptr, char **restrict endptr, int base, bool is_signed, uintmax_t max);

/* rand() state, one PCG32 generator per thread (docs/research/stdlib-part2.md) */
extern thread_local uint64_t __libc_rand_state;

#define __LIBC_RAND_MULTIPLIER 6364136223846793005ULL
#define __LIBC_RAND_INCREMENT 1442695040888963407ULL

/* Handler tables for atexit and at_quick_exit: the C23 minimum of 32 each, no allocation */
#define __LIBC_EXIT_HANDLERS 32

struct __libc_exit_handlers {
	unsigned int count;
	void (*handlers[__LIBC_EXIT_HANDLERS])(void);
};

/* Protected by __rtos_atexit_lock */
extern struct __libc_exit_handlers __libc_atexit_handlers;
extern struct __libc_exit_handlers __libc_quick_exit_handlers;

int __libc_exit_register(struct __libc_exit_handlers *table, void (*func)(void));
void __libc_exit_run(struct __libc_exit_handlers *table);

/* Runs the atexit handlers; linked only with atexit (a weak reference from exit, PLAN D52) */
[[gnu::weak]] void __libc_atexit_run(void);

#endif /* _INTERNAL_STDLIB_H */
