/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * string.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef __string_h
#define __string_h

/* C23 7.26 (partial: the remaining declarations arrive with P1.03) */
#define __STDC_VERSION_STRING_H__ 202311L

typedef __SIZE_TYPE__ size_t;

#define NULL ((void *)0)

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);

#endif
