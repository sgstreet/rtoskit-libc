/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * file.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_FILE_H
#define _INTERNAL_FILE_H

#include <stddef.h>
#include <stdio.h>

/* The stream object; grows into the full buffered stream in P1.09 */
struct __libc_file {
	int handle;
};

size_t __libc_file_write(FILE *stream, const void *buffer, size_t count);

#endif /* _INTERNAL_FILE_H */
