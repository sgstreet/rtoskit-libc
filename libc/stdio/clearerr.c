/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * clearerr.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.10.1 */
void clearerr(FILE *stream)
{
	__libc_file_lock(stream);
	stream->flags &= ~(__LIBC_FILE_EOF | __LIBC_FILE_ERROR);
	__libc_file_unlock(stream);
}
