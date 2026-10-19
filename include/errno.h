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

#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * C23 7.5. The values follow the Linux numbering so the Linux test port can
 * pass kernel error numbers through unchanged (PLAN D20).
 */
#define EDOM 33
#define ERANGE 34
#define EILSEQ 84

/* Additional error numbers (C23 7.5p4) */
#define ENOENT 2
#define EIO 5
#define EBADF 9
#define ENOMEM 12
#define EACCES 13
#define EEXIST 17
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define EMFILE 24
#define ENOSPC 28
#define ESPIPE 29
#define ENAMETOOLONG 36
#define ENOSYS 38
#define EOVERFLOW 75

/* C23 7.5p2: a modifiable lvalue of type int with thread storage duration */
extern thread_local int errno;
#define errno errno

#endif /* _ERRNO_H */
