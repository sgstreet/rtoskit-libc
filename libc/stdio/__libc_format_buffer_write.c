/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_buffer_write.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>

#include "internal/format.h"

/* Stores what fits and drops the rest; the room counts down, so no end pointer is formed */
void __libc_format_buffer_write(struct __libc_format_sink *sink, const char *s, size_t n)
{
	struct __libc_format_buffer *buffer = (struct __libc_format_buffer *)sink;
	size_t i;

	if (n > buffer->room)
		n = buffer->room;

	for (i = 0; i < n; ++i)
		buffer->pos[i] = s[i];

	buffer->pos += n;
	buffer->room -= n;
}
