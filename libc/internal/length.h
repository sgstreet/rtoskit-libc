/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * length.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_LENGTH_H
#define _INTERNAL_LENGTH_H

#include <stddef.h>
#include <stdint.h>

/*
 * The length modifiers of the printf and scanf families (C23 7.23.6.1p7,
 * 7.23.6.2p11), kept so va_arg, %n and each store use the exact type.
 */
enum __libc_length {
	__LIBC_LENGTH_NONE,
	__LIBC_LENGTH_HH,
	__LIBC_LENGTH_H,
	__LIBC_LENGTH_L,
	__LIBC_LENGTH_LL,
	__LIBC_LENGTH_J,
	__LIBC_LENGTH_Z,
	__LIBC_LENGTH_T,
	__LIBC_LENGTH_W8,
	__LIBC_LENGTH_W16,
	__LIBC_LENGTH_W32,
	__LIBC_LENGTH_W64,
	__LIBC_LENGTH_WF8,
	__LIBC_LENGTH_WF16,
	__LIBC_LENGTH_WF32,
	__LIBC_LENGTH_WF64,
	__LIBC_LENGTH_BIG_L,
};

/*
 * The modifiers naming a type wider than long, which the minimal flavour
 * refuses (libc-minimal-printf.a, PLAN D53). The sizes are constants, so the
 * test folds to the lengths that are wider on the target.
 */
static inline bool __libc_length_wider_than_long(enum __libc_length length)
{
	switch (length) {
		case __LIBC_LENGTH_LL:
			return sizeof(long long) > sizeof(long);
		case __LIBC_LENGTH_J:
			return sizeof(intmax_t) > sizeof(long);
		case __LIBC_LENGTH_Z:
			return sizeof(size_t) > sizeof(long);
		case __LIBC_LENGTH_T:
			return sizeof(ptrdiff_t) > sizeof(long);
		case __LIBC_LENGTH_W64:
			return sizeof(int64_t) > sizeof(long);
		case __LIBC_LENGTH_WF64:
			return sizeof(int_fast64_t) > sizeof(long);
		default:
			return false;
	}
}

#endif /* _INTERNAL_LENGTH_H */
