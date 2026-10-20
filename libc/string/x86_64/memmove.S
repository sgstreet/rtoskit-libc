/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memmove.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.2.3 for x86-64 with SSE2, written for speed (PLAN D76,
 * docs/research/string-speed.md). A destination below the source, or past
 * its end ((dst - src) >= n, unsigned), is a forward copy: memcpy, which
 * loads before it stores at every size. Otherwise, up to 32 bytes memcpy's
 * small paths serve as well (they load everything first); above 32 the
 * copy mirrors memcpy downwards: the first and last 16 source bytes are
 * loaded first and stored last, and between them the end of the
 * destination is aligned to 16 and 32 bytes per iteration go through
 * unaligned loads and aligned stores, moving down.
 */
	.text
	.globl memmove
	.type memmove, @function
memmove:
	movq %rdi, %rax
	subq %rsi, %rax
	cmpq %rdx, %rax
	jae memcpy
	cmpq $32, %rdx
	jbe memcpy
	movq %rdi, %rax
	movdqu (%rsi), %xmm2
	movdqu -16(%rsi,%rdx), %xmm3
	/* r9 = the destination end down to a 16-byte boundary (below dst + n - 16 ... dst + n - 1), rsi with it */
	leaq (%rdi,%rdx), %r9
	movq %r9, %rcx
	andq $-16, %r9
	subq %r9, %rcx
	leaq (%rsi,%rdx), %rsi
	subq %rcx, %rsi
	/* r8: the lowest aligned store starts at or above dst + 16 */
	leaq 16(%rdi), %r8
	leaq 32(%r8), %r10
1:
	cmpq %r10, %r9
	jb 2f
	movdqu -16(%rsi), %xmm0
	movdqu -32(%rsi), %xmm1
	movdqa %xmm0, -16(%r9)
	movdqa %xmm1, -32(%r9)
	subq $32, %rsi
	subq $32, %r9
	jmp 1b
	/* Less than 32 bytes above r8: 16 at a time; the head covers the rest */
2:
	cmpq %r8, %r9
	jbe 3f
	movdqu -16(%rsi), %xmm0
	movdqa %xmm0, -16(%r9)
	subq $16, %rsi
	subq $16, %r9
	jmp 2b
3:
	movdqu %xmm3, -16(%rdi,%rdx)
	movdqu %xmm2, (%rdi)
	ret
	.size memmove, . - memmove

	.section .note.GNU-stack, "", @progbits
