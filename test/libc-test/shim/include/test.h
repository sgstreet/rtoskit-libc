/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * test.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _TEST_H
#define _TEST_H

#include <stddef.h>
#include <stdint.h>

/*
 * Replacement for libc-test's src/common/test.h, which includes the POSIX
 * <unistd.h>. Only the helpers the enabled tests use are provided; the rest
 * arrive with the step that enables their users (see manifest.txt).
 */

extern volatile int t_status;

#define T_LOC2(l) __FILE__ ":" #l
#define T_LOC1(l) T_LOC2(l)
#define t_error(...) t_printf(T_LOC1(__LINE__) ": " __VA_ARGS__)

int t_printf(const char *s, ...);

/* Uses up the heap (P1.04) */
int t_memfill(void);

/* Pseudo-random helpers (P1.05) */
void t_randseed(uint64_t s);
uint64_t t_randn(uint64_t n);
uint64_t t_randint(uint64_t a, uint64_t b);
void t_shuffle(uint64_t *p, size_t n);
void t_randrange(uint64_t *p, size_t n);

/* POSIX environment functions (P1.11); tzset has nothing to do, as the library reads TZ at every conversion */
int setenv(const char *name, const char *value, int overwrite);
int putenv(char *string);
void tzset(void);

/* Adds or replaces the environment entry "name=value" held by entry */
int t_setenv(char *entry, size_t name_length, int overwrite);

#endif /* _TEST_H */
