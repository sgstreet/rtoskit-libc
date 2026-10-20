/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strlen.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.6.4 for x86-64 with SSE2, written for speed (PLAN D76,
 * docs/research/string-speed.md). Only 16-byte aligned blocks are read: an
 * aligned block never crosses a page, so the bytes before the string and
 * after its terminator that it holds cannot fault. PCMPEQB against zero and
 * PMOVMSKB give one bit per zero byte; in the first block the bits below
 * the string's start are shifted out; BSF finds the first zero. An empty
 * string returns before the setup.
 */
	.text
	.globl strlen
	.type strlen, @function
strlen:
	xorl %eax, %eax
	cmpb $0, (%rdi)
	je 3f
	pxor %xmm0, %xmm0
	movq %rdi, %rcx
	movq %rdi, %rax
	andq $-16, %rax
	andl $15, %ecx
	movdqa (%rax), %xmm1
	pcmpeqb %xmm0, %xmm1
	pmovmskb %xmm1, %edx
	shrl %cl, %edx
	testl %edx, %edx
	jnz 2f
1:
	addq $16, %rax
	movdqa (%rax), %xmm1
	pcmpeqb %xmm0, %xmm1
	pmovmskb %xmm1, %edx
	testl %edx, %edx
	jz 1b
	bsfl %edx, %edx
	addq %rdx, %rax
	subq %rdi, %rax
	ret
2:
	bsfl %edx, %eax
3:
	ret
	.size strlen, . - strlen

	.section .note.GNU-stack, "", @progbits
