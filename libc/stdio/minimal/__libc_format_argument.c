/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_argument.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/format.h"

/* C23 7.23.6.1p9: the promoted argument converted to the type the length modifier names */
static long fetch_signed(va_list *ap, enum __libc_length length)
{
	switch (length) {
		case __LIBC_LENGTH_HH:
			return (signed char)va_arg(*ap, int);
		case __LIBC_LENGTH_H:
			return (short)va_arg(*ap, int);
		case __LIBC_LENGTH_W8:
			return (int8_t)va_arg(*ap, int);
		case __LIBC_LENGTH_W16:
			return (int16_t)va_arg(*ap, int);
		case __LIBC_LENGTH_WF8:
			return (int_fast8_t)va_arg(*ap, typeof(+(int_fast8_t)0));
		case __LIBC_LENGTH_WF16:
			return (int_fast16_t)va_arg(*ap, typeof(+(int_fast16_t)0));
		case __LIBC_LENGTH_WF32:
			return (int_fast32_t)va_arg(*ap, typeof(+(int_fast32_t)0));
		case __LIBC_LENGTH_W32:
			return va_arg(*ap, int32_t);
		case __LIBC_LENGTH_L:
			return va_arg(*ap, long);
		case __LIBC_LENGTH_LL:
			return va_arg(*ap, long long);
		case __LIBC_LENGTH_J:
			return va_arg(*ap, intmax_t);
		case __LIBC_LENGTH_Z:
			return va_arg(*ap, typeof(_Generic((size_t)0, unsigned long: 0L, unsigned long long: 0LL, default: 0)));
		case __LIBC_LENGTH_T:
			return va_arg(*ap, ptrdiff_t);
		case __LIBC_LENGTH_W64:
			return va_arg(*ap, int64_t);
		case __LIBC_LENGTH_WF64:
			return va_arg(*ap, int_fast64_t);
		default:
			return va_arg(*ap, int);
	}
}

static unsigned long fetch_unsigned(va_list *ap, enum __libc_length length)
{
	switch (length) {
		case __LIBC_LENGTH_HH:
			return (unsigned char)va_arg(*ap, int);
		case __LIBC_LENGTH_H:
			return (unsigned short)va_arg(*ap, int);
		case __LIBC_LENGTH_W8:
			return (uint8_t)va_arg(*ap, int);
		case __LIBC_LENGTH_W16:
			return (uint16_t)va_arg(*ap, int);
		case __LIBC_LENGTH_WF8:
			return (uint_fast8_t)va_arg(*ap, typeof(+(uint_fast8_t)0));
		case __LIBC_LENGTH_WF16:
			return (uint_fast16_t)va_arg(*ap, typeof(+(uint_fast16_t)0));
		case __LIBC_LENGTH_WF32:
			return (uint_fast32_t)va_arg(*ap, typeof(+(uint_fast32_t)0));
		case __LIBC_LENGTH_W32:
			return va_arg(*ap, uint32_t);
		case __LIBC_LENGTH_L:
			return va_arg(*ap, unsigned long);
		case __LIBC_LENGTH_LL:
			return va_arg(*ap, unsigned long long);
		case __LIBC_LENGTH_J:
			return va_arg(*ap, uintmax_t);
		case __LIBC_LENGTH_Z:
			return va_arg(*ap, size_t);
		case __LIBC_LENGTH_T:
			return (typeof(_Generic((ptrdiff_t)0, long: 0UL, long long: 0ULL, default: 0U)))va_arg(*ap, ptrdiff_t);
		case __LIBC_LENGTH_W64:
			return va_arg(*ap, uint64_t);
		case __LIBC_LENGTH_WF64:
			return va_arg(*ap, uint_fast64_t);
		default:
			return va_arg(*ap, unsigned int);
	}
}

bool __libc_format_argument(va_list *ap, enum __libc_length length, bool is_signed, uintmax_t *magnitude, bool *negative)
{
	/* Lengths wider than long are refused, which keeps 64-bit division out of 32-bit images (PLAN D53) */
	if (__libc_length_wider_than_long(length)) {
		errno = EINVAL;
		return false;
	}

	if (is_signed) {
		long value = fetch_signed(ap, length);

		/* Negate in the unsigned type so LONG_MIN is representable */
		*negative = value < 0;
		*magnitude = *negative ? -(unsigned long)value : (unsigned long)value;
	} else {
		*negative = false;
		*magnitude = fetch_unsigned(ap, length);
	}

	return true;
}
