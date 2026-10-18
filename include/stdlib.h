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

#ifndef __stdlib_h
#define __stdlib_h

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

#endif
