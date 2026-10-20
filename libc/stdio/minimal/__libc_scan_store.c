/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_scan_store.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/scan.h"

/* Converts value to the type the modifier names; out-of-range values wrap as the conversion does */
/* libc-minimal-printf.a: the stores wider than long are refused, as printf refuses their arguments (PLAN D53) */
bool __libc_scan_store(va_list *ap, enum __libc_length length, uintmax_t value)
{
	if (__libc_length_wider_than_long(length)) {
		errno = EINVAL;
		return false;
	}

	switch (length) {
		case __LIBC_LENGTH_HH:
			*va_arg(*ap, unsigned char *) = (unsigned char)value;
			break;
		case __LIBC_LENGTH_H:
			*va_arg(*ap, unsigned short *) = (unsigned short)value;
			break;
		case __LIBC_LENGTH_L:
			*va_arg(*ap, unsigned long *) = (unsigned long)value;
			break;
		case __LIBC_LENGTH_LL:
			*va_arg(*ap, unsigned long long *) = (unsigned long long)value;
			break;
		case __LIBC_LENGTH_J:
			*va_arg(*ap, uintmax_t *) = value;
			break;
		case __LIBC_LENGTH_Z:
			*va_arg(*ap, size_t *) = (size_t)value;
			break;
		case __LIBC_LENGTH_T:
			*va_arg(*ap, ptrdiff_t *) = (ptrdiff_t)value;
			break;
		case __LIBC_LENGTH_W8:
			*va_arg(*ap, uint_least8_t *) = (uint_least8_t)value;
			break;
		case __LIBC_LENGTH_W16:
			*va_arg(*ap, uint_least16_t *) = (uint_least16_t)value;
			break;
		case __LIBC_LENGTH_W32:
			*va_arg(*ap, uint_least32_t *) = (uint_least32_t)value;
			break;
		case __LIBC_LENGTH_W64:
			*va_arg(*ap, uint_least64_t *) = (uint_least64_t)value;
			break;
		case __LIBC_LENGTH_WF8:
			*va_arg(*ap, uint_fast8_t *) = (uint_fast8_t)value;
			break;
		case __LIBC_LENGTH_WF16:
			*va_arg(*ap, uint_fast16_t *) = (uint_fast16_t)value;
			break;
		case __LIBC_LENGTH_WF32:
			*va_arg(*ap, uint_fast32_t *) = (uint_fast32_t)value;
			break;
		case __LIBC_LENGTH_WF64:
			*va_arg(*ap, uint_fast64_t *) = (uint_fast64_t)value;
			break;
		default:
			*va_arg(*ap, unsigned int *) = (unsigned int)value;
			break;
	}

	return true;
}
