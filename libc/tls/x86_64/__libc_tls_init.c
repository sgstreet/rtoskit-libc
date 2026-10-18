/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_tls_init.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "internal/tls.h"
#include "tls-arch.h"

void *__libc_tls_init(void *block, const struct __libc_tls_image *image)
{
	size_t align = __libc_tls_segment_align(image);

	if (align < __LIBC_TCB_ALIGN)
		align = __LIBC_TCB_ALIGN;

	/* The lowest thread pointer that leaves room for the TLS data below it */
	size_t offset = __libc_tls_variant2_offset(image);
	uintptr_t tp = __libc_tls_align_up((uintptr_t)block + offset, align);

	__libc_tls_copy_image((unsigned char *)(tp - offset), image);

	struct __libc_tcb *tcb = (struct __libc_tcb *)tp;

	tcb->self = tcb;
	for (size_t i = 0; i < sizeof(tcb->reserved) / sizeof(tcb->reserved[0]); ++i)
		tcb->reserved[i] = 0;

	return tcb;
}
