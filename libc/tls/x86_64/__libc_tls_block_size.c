/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tls_block_size.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/tls.h"
#include "tls-arch.h"

size_t __libc_tls_block_size(const struct __libc_tls_image *image)
{
	size_t align = __libc_tls_segment_align(image);
	if (align < __libc_tcb_align)
		align = __libc_tcb_align;

	/* Slack for aligning the thread pointer anywhere inside the block */
	return __libc_tls_variant2_offset(image) + sizeof(struct __libc_tcb) + align - 1;
}
