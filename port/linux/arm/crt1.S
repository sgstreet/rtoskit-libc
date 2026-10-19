/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * crt1.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Process entry (Thumb; the kernel starts Thumb code when the ELF entry
 * address has bit 0 set). The kernel leaves argc at the stack pointer,
 * followed by the argv and envp arrays, with the stack 8-byte aligned as
 * AAPCS requires at a call. Clear lr so backtraces end here. Thumb-1 only,
 * so the same code serves every M-profile architecture.
 */
	.syntax unified
	.thumb
	.text
	.global _start
	.type _start, %function
_start:
	movs r0, #0
	mov lr, r0
	mov r0, sp
	bl __rtos_linux_start
	udf #0
	.size _start, . - _start

	.section .note.GNU-stack, "", %progbits
