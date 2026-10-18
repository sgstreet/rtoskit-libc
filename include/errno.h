/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * errno.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef __errno_h
#define __errno_h

/*
 * C23 7.5. The values follow the Linux numbering so the Linux test port can
 * pass kernel error numbers through unchanged (PLAN D20).
 */
#define EDOM 33
#define ERANGE 34
#define EILSEQ 84

/* Additional error numbers (C23 7.5p4) */
#define EBADF 9

/* C23 7.5p2: a modifiable lvalue of type int with thread storage duration */
extern thread_local int errno;
#define errno errno

#endif
