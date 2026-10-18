/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdio.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef __stdio_h
#define __stdio_h

/* C23 7.23 (partial: the streams are completed in P1.09) */
#define __STDC_VERSION_STDIO_H__ 202311L

typedef __SIZE_TYPE__ size_t;

#define NULL ((void *)0)

#define EOF (-1)

typedef struct __libc_file FILE;

extern FILE __libc_stdin;
extern FILE __libc_stdout;
extern FILE __libc_stderr;

#define stdin (&__libc_stdin)
#define stdout (&__libc_stdout)
#define stderr (&__libc_stderr)

int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);

/* Declared for the compiler's printf-to-puts folding; implemented in P1.09 */
int printf(const char *restrict format, ...);

#endif
