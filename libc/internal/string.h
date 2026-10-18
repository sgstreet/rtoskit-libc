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

#ifndef _INTERNAL_STRING_H
#define _INTERNAL_STRING_H

/* Returns a pointer to the first c (converted to unsigned char) in s, or to its terminator */
char *__libc_strchrnul(const char *s, int c);

#endif /* _INTERNAL_STRING_H */
