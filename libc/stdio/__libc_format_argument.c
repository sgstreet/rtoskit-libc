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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/format.h"

/*
 * C23 7.23.6.1p9: fetch the promoted argument, then convert it to the type
 * the length modifier names. The magnitude is returned with the sign apart.
 */
static uintmax_t fetch_signed(va_list *ap, enum __libc_length length, bool *negative)
{
	intmax_t value;

	switch (length) {
		case __LIBC_LENGTH_HH:
			value = (signed char)va_arg(*ap, int);
			break;
		case __LIBC_LENGTH_H:
			value = (short)va_arg(*ap, int);
			break;
		case __LIBC_LENGTH_L:
			value = va_arg(*ap, long);
			break;
		case __LIBC_LENGTH_LL:
			value = va_arg(*ap, long long);
			break;
		case __LIBC_LENGTH_J:
			value = va_arg(*ap, intmax_t);
			break;
		case __LIBC_LENGTH_Z:
			value = va_arg(*ap, typeof(_Generic((size_t)0, unsigned long: 0L, unsigned long long: 0LL, default: 0)));
			break;
		case __LIBC_LENGTH_T:
			value = va_arg(*ap, ptrdiff_t);
			break;
		case __LIBC_LENGTH_W8:
			value = (int8_t)va_arg(*ap, int);
			break;
		case __LIBC_LENGTH_W16:
			value = (int16_t)va_arg(*ap, int);
			break;
		case __LIBC_LENGTH_W32:
			value = va_arg(*ap, int32_t);
			break;
		case __LIBC_LENGTH_W64:
			value = va_arg(*ap, int64_t);
			break;
		case __LIBC_LENGTH_WF8:
			value = (int_fast8_t)va_arg(*ap, typeof(+(int_fast8_t)0));
			break;
		case __LIBC_LENGTH_WF16:
			value = (int_fast16_t)va_arg(*ap, typeof(+(int_fast16_t)0));
			break;
		case __LIBC_LENGTH_WF32:
			value = (int_fast32_t)va_arg(*ap, typeof(+(int_fast32_t)0));
			break;
		case __LIBC_LENGTH_WF64:
			value = va_arg(*ap, int_fast64_t);
			break;
		default:
			value = va_arg(*ap, int);
			break;
	}

	*negative = value < 0;

	/* Negate in the unsigned type so INTMAX_MIN is representable */
	return *negative ? -(uintmax_t)value : (uintmax_t)value;
}

static uintmax_t fetch_unsigned(va_list *ap, enum __libc_length length)
{
	switch (length) {
		case __LIBC_LENGTH_HH:
			return (unsigned char)va_arg(*ap, int);
		case __LIBC_LENGTH_H:
			return (unsigned short)va_arg(*ap, int);
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
		case __LIBC_LENGTH_W8:
			return (uint8_t)va_arg(*ap, int);
		case __LIBC_LENGTH_W16:
			return (uint16_t)va_arg(*ap, int);
		case __LIBC_LENGTH_W32:
			return va_arg(*ap, uint32_t);
		case __LIBC_LENGTH_W64:
			return va_arg(*ap, uint64_t);
		case __LIBC_LENGTH_WF8:
			return (uint_fast8_t)va_arg(*ap, typeof(+(uint_fast8_t)0));
		case __LIBC_LENGTH_WF16:
			return (uint_fast16_t)va_arg(*ap, typeof(+(uint_fast16_t)0));
		case __LIBC_LENGTH_WF32:
			return (uint_fast32_t)va_arg(*ap, typeof(+(uint_fast32_t)0));
		case __LIBC_LENGTH_WF64:
			return va_arg(*ap, uint_fast64_t);
		default:
			return va_arg(*ap, unsigned int);
	}
}

/* Every length modifier is supported */
bool __libc_format_argument(va_list *ap, enum __libc_length length, bool is_signed, uintmax_t *magnitude, bool *negative)
{
	if (is_signed) {
		*magnitude = fetch_signed(ap, length, negative);
	} else {
		*negative = false;
		*magnitude = fetch_unsigned(ap, length);
	}

	return true;
}
