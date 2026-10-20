/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memcpy.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.2.1 for x86-64 with SSE2 (the baseline ISA), written for speed
 * (PLAN D76, docs/research/string-speed.md). Every size loads what it needs
 * before it stores anything that could overlap a later load, so memmove
 * uses this function for any forward copy:
 *   0-15 bytes: two overlapping loads of 8, 4, 2 or 1 bytes, from the head
 *     and the tail, then the two stores;
 *   16-32 bytes: two overlapping unaligned 16-byte loads and stores;
 *   above 32: the first and last 16 bytes are loaded first and stored last;
 *     between them the destination is aligned to 16 and 32 bytes per
 *     iteration go through unaligned loads and aligned stores, moving up.
 */
	.text
	.globl memcpy
	.type memcpy, @function
memcpy:
	movq %rdi, %rax
	cmpq $16, %rdx
	jb .Lsmall
	cmpq $32, %rdx
	ja .Lbig
	movdqu (%rsi), %xmm0
	movdqu -16(%rsi,%rdx), %xmm1
	movdqu %xmm0, (%rdi)
	movdqu %xmm1, -16(%rdi,%rdx)
	ret

.Lsmall:
	cmpq $8, %rdx
	jb 1f
	movq (%rsi), %rcx
	movq -8(%rsi,%rdx), %r8
	movq %rcx, (%rdi)
	movq %r8, -8(%rdi,%rdx)
	ret
1:
	cmpq $4, %rdx
	jb 2f
	movl (%rsi), %ecx
	movl -4(%rsi,%rdx), %r8d
	movl %ecx, (%rdi)
	movl %r8d, -4(%rdi,%rdx)
	ret
2:
	testq %rdx, %rdx
	je 3f
	movzbl (%rsi), %ecx
	movzbl -1(%rsi,%rdx), %r8d
	cmpq $2, %rdx
	jb 4f
	movzwl (%rsi), %ecx
	movzwl -2(%rsi,%rdx), %r8d
	movw %cx, (%rdi)
	movw %r8w, -2(%rdi,%rdx)
	ret
4:
	movb %cl, (%rdi)
3:
	ret

.Lbig:
	movdqu (%rsi), %xmm2
	movdqu -16(%rsi,%rdx), %xmm3
	/* r9: the end of the destination; rdi up to the next 16-byte boundary past it, rsi with it */
	leaq (%rdi,%rdx), %r9
	movq %rdi, %rcx
	andq $-16, %rdi
	addq $16, %rdi
	subq %rcx, %rsi
	addq %rdi, %rsi
	/* r8: the end of the aligned stores, 16 short of the destination end */
	leaq -16(%r9), %r8
	leaq -32(%r8), %r10
	cmpq %r10, %rdi
	ja 6f
5:
	movdqu (%rsi), %xmm0
	movdqu 16(%rsi), %xmm1
	movdqa %xmm0, (%rdi)
	movdqa %xmm1, 16(%rdi)
	addq $32, %rsi
	addq $32, %rdi
	cmpq %r10, %rdi
	jbe 5b
	/* Less than 32 bytes to r8: 16 at a time (twice at most); the tail covers the rest */
6:
	cmpq %r8, %rdi
	jae 7f
	movdqu (%rsi), %xmm0
	movdqa %xmm0, (%rdi)
	addq $16, %rsi
	addq $16, %rdi
	jmp 6b
7:
	movdqu %xmm2, (%rax)
	movdqu %xmm3, -16(%r9)
	ret
	.size memcpy, . - memcpy

	.section .note.GNU-stack, "", @progbits
