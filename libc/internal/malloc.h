/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_MALLOC_H
#define _INTERNAL_MALLOC_H

/*
 * Services every allocator variant (libc/malloc/<variant>, chosen with
 * LIBC_MALLOC, PLAN D32) provides beyond the standard functions.
 */

/* Verifies every invariant of the heap; takes the lock itself (for tests) */
bool __libc_malloc_check(void);

#endif /* _INTERNAL_MALLOC_H */
