/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_count.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/format.h"

/* C23 7.23.6.1p8 n: store the count through a pointer of the type the length modifier names */
bool __libc_format_count(va_list *ap, enum __libc_length length, size_t count)
{
	switch (length) {
		case __LIBC_LENGTH_HH:
			*va_arg(*ap, signed char *) = count;
			break;
		case __LIBC_LENGTH_H:
			*va_arg(*ap, short *) = count;
			break;
		case __LIBC_LENGTH_L:
			*va_arg(*ap, long *) = count;
			break;
		case __LIBC_LENGTH_LL:
			*va_arg(*ap, long long *) = count;
			break;
		case __LIBC_LENGTH_J:
			*va_arg(*ap, intmax_t *) = count;
			break;
		case __LIBC_LENGTH_Z:
			*va_arg(*ap, typeof(_Generic((size_t)0, unsigned long: 0L, unsigned long long: 0LL, default: 0)) *) = count;
			break;
		case __LIBC_LENGTH_T:
			*va_arg(*ap, ptrdiff_t *) = count;
			break;
		case __LIBC_LENGTH_W8:
			*va_arg(*ap, int8_t *) = count;
			break;
		case __LIBC_LENGTH_W16:
			*va_arg(*ap, int16_t *) = count;
			break;
		case __LIBC_LENGTH_W32:
			*va_arg(*ap, int32_t *) = count;
			break;
		case __LIBC_LENGTH_W64:
			*va_arg(*ap, int64_t *) = count;
			break;
		case __LIBC_LENGTH_WF8:
			*va_arg(*ap, int_fast8_t *) = count;
			break;
		case __LIBC_LENGTH_WF16:
			*va_arg(*ap, int_fast16_t *) = count;
			break;
		case __LIBC_LENGTH_WF32:
			*va_arg(*ap, int_fast32_t *) = count;
			break;
		case __LIBC_LENGTH_WF64:
			*va_arg(*ap, int_fast64_t *) = count;
			break;
		default:
			*va_arg(*ap, int *) = count;
			break;
	}

	return true;
}
