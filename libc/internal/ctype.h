/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ctype.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_CTYPE_H
#define _INTERNAL_CTYPE_H

/*
 * "C" locale classification without a table (docs/research/string-ctype-stdbit.md).
 * The unsigned subtraction folds each range test into one comparison and makes
 * EOF (negative) fail every test.
 */

static inline bool __libc_isdigit(int c)
{
	return (unsigned int)c - '0' < 10;
}

static inline bool __libc_isupper(int c)
{
	return (unsigned int)c - 'A' < 26;
}

static inline bool __libc_islower(int c)
{
	return (unsigned int)c - 'a' < 26;
}

/* Setting bit 5 maps 'A'..'Z' onto 'a'..'z' and moves no other value into that range */
static inline bool __libc_isalpha(int c)
{
	return ((unsigned int)c | 0x20) - 'a' < 26;
}

static inline bool __libc_isalnum(int c)
{
	return __libc_isalpha(c) || __libc_isdigit(c);
}

/* Printing characters other than space: 0x21 to 0x7e */
static inline bool __libc_isgraph(int c)
{
	return (unsigned int)c - 0x21 < 0x5e;
}

#endif /* _INTERNAL_CTYPE_H */
