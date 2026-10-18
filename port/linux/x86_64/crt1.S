/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * crt1.S
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Process entry. The kernel leaves argc at the stack pointer, followed by the
 * argv and envp arrays. Pass that address to the C startup code with the
 * stack realigned to 16 bytes as the ABI requires at a call.
 */
	.text
	.global _start
	.type _start, @function
_start:
	xor %ebp, %ebp
	mov %rsp, %rdi
	and $-16, %rsp
	call __rtos_linux_start
	hlt
	.size _start, . - _start

	.section .note.GNU-stack, "", @progbits
