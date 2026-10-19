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

#ifndef _STDLIB_H
#define _STDLIB_H

/* C23 7.24 (partial: the remaining declarations arrive with P1.04 and P1.05) */
#define __STDC_VERSION_STDLIB_H__ 202311L

typedef __SIZE_TYPE__ size_t;
typedef __WCHAR_TYPE__ wchar_t;

#define NULL ((void *)0)

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

/* C23 7.24.4 communication with the environment */
[[noreturn]] void abort(void);
[[noreturn]] void exit(int status);
[[noreturn]] void _Exit(int status);

/* Memory management functions (C23 7.24.3) */
void *aligned_alloc(size_t alignment, size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
void free_sized(void *ptr, size_t size);
void free_aligned_sized(void *ptr, size_t alignment, size_t size);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

/* Alignment of memory (C23 7.24.9) */
size_t memalignment(const void *p);

#endif /* _STDLIB_H */
