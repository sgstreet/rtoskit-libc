/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strchr.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.5.2 for x86-64 with SSE2, written for speed (PLAN D76,
 * docs/research/string-speed.md). The character converts to char (unsigned
 * here, 7.26.5.2p2) and is broadcast to an XMM register. Only 16-byte
 * aligned blocks are read (none crosses a page). PCMPEQB against zero and
 * against the character, POR and PMOVMSKB give one bit per byte that ends
 * the search; the bits below the string's start are shifted out of the
 * first block. The first such byte is the character (its address is the
 * result) or the terminator (a null pointer, unless the character is the
 * terminator itself). The first two bytes are tested one at a time before
 * the character is broadcast: short strings and early matches end there.
 */
	.text
	.globl strchr
	.type strchr, @function
strchr:
	movzbl %sil, %esi
	movzbl (%rdi), %eax
	cmpl %esi, %eax
	je 5f
	testl %eax, %eax
	jz 6f
	movzbl 1(%rdi), %eax
	cmpl %esi, %eax
	je 7f
	testl %eax, %eax
	jz 6f
	addq $2, %rdi
	movd %esi, %xmm2
	punpcklbw %xmm2, %xmm2
	punpcklwd %xmm2, %xmm2
	pshufd $0, %xmm2, %xmm2
	pxor %xmm0, %xmm0
	movq %rdi, %rcx
	movq %rdi, %rax
	andq $-16, %rax
	andl $15, %ecx
	movdqa (%rax), %xmm1
	movdqa %xmm1, %xmm3
	pcmpeqb %xmm0, %xmm1
	pcmpeqb %xmm2, %xmm3
	por %xmm3, %xmm1
	pmovmskb %xmm1, %edx
	shrl %cl, %edx
	testl %edx, %edx
	jz 1f
	bsfl %edx, %edx
	leaq (%rdi,%rdx), %rax
	jmp 3f
1:
	addq $16, %rax
	movdqa (%rax), %xmm1
	movdqa %xmm1, %xmm3
	pcmpeqb %xmm0, %xmm1
	pcmpeqb %xmm2, %xmm3
	por %xmm3, %xmm1
	pmovmskb %xmm1, %edx
	testl %edx, %edx
	jz 1b
	bsfl %edx, %edx
	addq %rdx, %rax
3:
	cmpb %sil, (%rax)
	je 4f
	xorl %eax, %eax
4:
	ret
5:
	movq %rdi, %rax
	ret
7:
	leaq 1(%rdi), %rax
	ret
6:
	xorl %eax, %eax
	ret
	.size strchr, . - strchr

	.section .note.GNU-stack, "", @progbits
