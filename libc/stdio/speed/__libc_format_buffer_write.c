/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_buffer_write.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <string.h>

#include "internal/format.h"

/*
 * The speed flavour's buffer sink (PLAN D77): what fits goes through
 * memcpy, the flavour's block copy, from 16 bytes; shorter pieces (a
 * conversion's few characters, padding) are cheaper copied in place than
 * through a call.
 */
void __libc_format_buffer_write(struct __libc_format_sink *sink, const char *s, size_t n)
{
	struct __libc_format_buffer *buffer = (struct __libc_format_buffer *)sink;
	char *pos = buffer->pos;

	if (n > buffer->room)
		n = buffer->room;
	buffer->pos = pos + n;
	buffer->room -= n;

	/* The buffer is updated first, so the copy is a tail call */
	if (n >= 16) {
		memcpy(pos, s, n);
		return;
	}
	for (size_t i = 0; i < n; ++i)
		pos[i] = s[i];
}
