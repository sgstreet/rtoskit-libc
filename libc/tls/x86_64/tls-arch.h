/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tls-arch.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef __internal_tls_arch_h
#define __internal_tls_arch_h

#include <stddef.h>
#include <stdint.h>

/*
 * x86_64 uses TLS variant II: the thread pointer (%fs base) addresses the
 * thread control block and the TLS data sits directly below it. Compilers
 * read %fs:0 to obtain the thread pointer itself, so the first word of the
 * control block must point to itself. The block is 64 bytes so code built
 * with -fstack-protector, which reads its guard at %fs:0x28, finds zeroed
 * memory instead of faulting.
 */
struct __libc_tcb {
	struct __libc_tcb *self;
	uintptr_t reserved[7];
};

/* Thread pointer alignment; the control block holds pointers and 16 suits SSE spills */
static constexpr size_t __libc_tcb_align = 16;

#endif
