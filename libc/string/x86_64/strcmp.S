/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strcmp.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.4.2 for x86-64 with SSE2, written for speed (PLAN D76,
 * docs/research/string-speed.md). 16 bytes of each string per iteration
 * through unaligned loads, as long as neither load can cross into the next
 * 4 KB page (page offset at most 4080): a page the strings do not reach may
 * not exist. Near a page end one byte pair is compared and the loop tries
 * again. PCMPEQB of the two blocks gives 0xff for the equal bytes; PMINUB
 * with the first block turns that into the first string's byte where equal
 * and 0 where not, so PCMPEQB against zero marks exactly the bytes that
 * differ or end the first string. The first (BSF) gives the result, the
 * difference of the two bytes there as unsigned char (7.26.4p1). The first eight byte
 * pairs are compared one at a time before any of that (5 instructions a
 * pair): most comparisons end there (a difference in the first characters,
 * short strings), and by the eighth the bytes have saved what the block
 * setup costs.
 */
	.text
	.globl strcmp
	.type strcmp, @function
strcmp:
	movzbl 0(%rdi), %eax
	cmpb 0(%rsi), %al
	jne 10f
	testb %al, %al
	jz 10f
	movzbl 1(%rdi), %eax
	cmpb 1(%rsi), %al
	jne 11f
	testb %al, %al
	jz 11f
	movzbl 2(%rdi), %eax
	cmpb 2(%rsi), %al
	jne 12f
	testb %al, %al
	jz 12f
	movzbl 3(%rdi), %eax
	cmpb 3(%rsi), %al
	jne 13f
	testb %al, %al
	jz 13f
	movzbl 4(%rdi), %eax
	cmpb 4(%rsi), %al
	jne 14f
	testb %al, %al
	jz 14f
	movzbl 5(%rdi), %eax
	cmpb 5(%rsi), %al
	jne 15f
	testb %al, %al
	jz 15f
	movzbl 6(%rdi), %eax
	cmpb 6(%rsi), %al
	jne 16f
	testb %al, %al
	jz 16f
	movzbl 7(%rdi), %eax
	cmpb 7(%rsi), %al
	jne 17f
	testb %al, %al
	jz 17f
	addq $8, %rdi
	addq $8, %rsi
	pxor %xmm0, %xmm0
1:
	movl %edi, %eax
	andl $4095, %eax
	cmpl $4080, %eax
	ja 3f
	movl %esi, %eax
	andl $4095, %eax
	cmpl $4080, %eax
	ja 3f
	movdqu (%rdi), %xmm1
	movdqu (%rsi), %xmm2
	pcmpeqb %xmm1, %xmm2
	pminub %xmm1, %xmm2
	pcmpeqb %xmm0, %xmm2
	pmovmskb %xmm2, %eax
	testl %eax, %eax
	jnz 2f
	addq $16, %rdi
	addq $16, %rsi
	jmp 1b
2:
	bsfl %eax, %ecx
	movzbl (%rdi,%rcx), %eax
	movzbl (%rsi,%rcx), %edx
	subl %edx, %eax
	ret
	/* One byte pair near a page end */
3:
	movzbl (%rdi), %eax
	movzbl (%rsi), %edx
	subl %edx, %eax
	jnz 4f
	testl %edx, %edx
	jz 4f
	addq $1, %rdi
	addq $1, %rsi
	jmp 1b
4:
	ret
	/* The prologue's ends: eax holds the first string's byte at offset k */
10:
	movzbl 0(%rsi), %edx
	subl %edx, %eax
	ret
11:
	movzbl 1(%rsi), %edx
	subl %edx, %eax
	ret
12:
	movzbl 2(%rsi), %edx
	subl %edx, %eax
	ret
13:
	movzbl 3(%rsi), %edx
	subl %edx, %eax
	ret
14:
	movzbl 4(%rsi), %edx
	subl %edx, %eax
	ret
15:
	movzbl 5(%rsi), %edx
	subl %edx, %eax
	ret
16:
	movzbl 6(%rsi), %edx
	subl %edx, %eax
	ret
17:
	movzbl 7(%rsi), %edx
	subl %edx, %eax
	ret
	.size strcmp, . - strcmp

	.section .note.GNU-stack, "", @progbits
