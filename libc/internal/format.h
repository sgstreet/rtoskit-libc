/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * format.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_FORMAT_H
#define _INTERNAL_FORMAT_H

#include <stdarg.h>
#include <stddef.h>

/* Conversion flags (C23 7.23.6.1p6) */
#define __LIBC_FORMAT_LEFT 0x01
#define __LIBC_FORMAT_PLUS 0x02
#define __LIBC_FORMAT_SPACE 0x04
#define __LIBC_FORMAT_ALT 0x08
#define __LIBC_FORMAT_ZERO 0x10

/* Destination of formatted output; write() receives the output in chunks */
struct __libc_format_sink {
	void (*write)(struct __libc_format_sink *sink, const char *s, size_t n);
};

/* A sink storing into a caller buffer, keeping at most room characters */
struct __libc_format_buffer {
	struct __libc_format_sink sink;
	char *pos;
	size_t room;
};

/* One conversion specification; precision is negative when omitted */
struct __libc_format_spec {
	unsigned int flags;
	int width;
	int precision;
	char conversion;
};

/* Returns the number of characters produced, or a negative value with errno set */
int __libc_format(struct __libc_format_sink *sink, const char *format, va_list ap);

/*
 * Formats one a/A/e/E/f/F/g/G conversion of value to the sink and returns the
 * number of characters produced, or a negative value with errno set. A single
 * replaceable symbol so a no-float flavour can drop it (PLAN D13).
 */
int __libc_format_float(struct __libc_format_sink *sink, const struct __libc_format_spec *spec, long double value);

void __libc_format_buffer_write(struct __libc_format_sink *sink, const char *s, size_t n);

#endif /* _INTERNAL_FORMAT_H */
