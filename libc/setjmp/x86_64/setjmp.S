/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setjmp.S
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.13.1.1 for x86_64 (System V psABI 3.2): save the callee-saved
 * registers, the stack pointer the caller will have after the return, and
 * the return address. jmp_buf layout: rbx, rbp, r12, r13, r14, r15, rsp, rip.
 */
	.text
	.global setjmp
	.type setjmp, @function
setjmp:
	mov %rbx, 0(%rdi)
	mov %rbp, 8(%rdi)
	mov %r12, 16(%rdi)
	mov %r13, 24(%rdi)
	mov %r14, 32(%rdi)
	mov %r15, 40(%rdi)
	lea 8(%rsp), %rdx
	mov %rdx, 48(%rdi)
	mov (%rsp), %rdx
	mov %rdx, 56(%rdi)
	xor %eax, %eax
	ret
	.size setjmp, . - setjmp

	.section .note.GNU-stack, "", @progbits
