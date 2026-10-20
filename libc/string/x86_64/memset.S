/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memset.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.6.1 for x86-64 with SSE2, written for speed (PLAN D76,
 * docs/research/string-speed.md): the byte replicated into a 64-bit word
 * (times 0x0101010101010101) and into an XMM register; below 16 bytes two
 * overlapping stores of 8, 4, 2 or 1 bytes from the head and the tail; 16
 * to 32 bytes two overlapping 16-byte stores; above 32 the first and last 16
 * bytes unaligned and 32 bytes per iteration aligned between them. Zero
 * bytes return before the byte is replicated.
 */
	.text
	.globl memset
	.type memset, @function
memset:
	movq %rdi, %rax
	testq %rdx, %rdx
	je 3f
	movzbl %sil, %ecx
	movabsq $0x0101010101010101, %r8
	imulq %r8, %rcx
	cmpq $16, %rdx
	jb .Lsmall
	movq %rcx, %xmm0
	punpcklqdq %xmm0, %xmm0
	cmpq $32, %rdx
	ja .Lbig
	movdqu %xmm0, (%rdi)
	movdqu %xmm0, -16(%rdi,%rdx)
	ret

.Lsmall:
	cmpq $8, %rdx
	jb 1f
	movq %rcx, (%rdi)
	movq %rcx, -8(%rdi,%rdx)
	ret
1:
	cmpq $4, %rdx
	jb 2f
	movl %ecx, (%rdi)
	movl %ecx, -4(%rdi,%rdx)
	ret
2:
	movb %cl, (%rdi)
	movb %cl, -1(%rdi,%rdx)
	cmpq $3, %rdx
	jb 3f
	movb %cl, 1(%rdi)
3:
	ret

.Lbig:
	movdqu %xmm0, (%rdi)
	movdqu %xmm0, -16(%rdi,%rdx)
	leaq -16(%rdi,%rdx), %r8
	andq $-16, %rdi
	addq $16, %rdi
	leaq -32(%r8), %r9
	cmpq %r9, %rdi
	ja 5f
4:
	movdqa %xmm0, (%rdi)
	movdqa %xmm0, 16(%rdi)
	addq $32, %rdi
	cmpq %r9, %rdi
	jbe 4b
5:
	cmpq %r8, %rdi
	jae 6f
	movdqa %xmm0, (%rdi)
	addq $16, %rdi
	jmp 5b
6:
	ret
	.size memset, . - memset

	.section .note.GNU-stack, "", @progbits
