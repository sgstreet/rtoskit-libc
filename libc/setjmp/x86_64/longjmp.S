/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * longjmp.S
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.13.2.1 for x86_64: restore what setjmp saved and return from that
 * setjmp again, with val, or 1 when val is 0 (p4).
 */
	.text
	.global longjmp
	.type longjmp, @function
longjmp:
	mov %esi, %eax
	test %eax, %eax
	jnz 1f
	inc %eax
1:
	mov 0(%rdi), %rbx
	mov 8(%rdi), %rbp
	mov 16(%rdi), %r12
	mov 24(%rdi), %r13
	mov 32(%rdi), %r14
	mov 40(%rdi), %r15
	mov 48(%rdi), %rsp
	jmp *56(%rdi)
	.size longjmp, . - longjmp

	.section .note.GNU-stack, "", @progbits
