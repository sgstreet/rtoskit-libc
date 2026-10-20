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
 * C23 7.26.4.2 for Thumb-2 (Armv7-M, Armv7E-M, Armv8-M Mainline), written
 * for speed (PLAN D74, docs/research/string-speed.md). The first 16
 * characters go through a byte loop unrolled four times, 6 instructions a
 * byte (one compare against 1 clears C at the terminator, a conditional
 * compare sets Z for equal bytes); by then it has saved more than the word
 * loop's entry and exit cost. Then bytes up to a word boundary of the first
 * string and, if the second shares its alignment phase, a word of each per
 * iteration: (a - 0x01010101) & ~a & 0x80808080 flags the first string's
 * terminator (its lowest set bit exactly; Thumb-2 encodes the constants as
 * immediates) and a ^ b every difference, so their OR is zero exactly while
 * the words are equal and the string goes on. Only aligned words are read,
 * none past the one holding either terminator (one of the second string
 * alone shows up as a difference). REV and CLZ of the flags give 8 times
 * the index of the first flagged byte, which is reloaded from both strings:
 * the result is their difference as unsigned char (7.26.4p1). Strings of
 * different phase stay in the byte loop. The DSP UADD8/SEL form needs a
 * sixth register, whose push costs more than the instruction it saves per
 * word on the short strings most programs compare.
 */
	.syntax unified
	.thumb
	.text
	.global strcmp
	.type strcmp, %function
strcmp:
	/* The byte steps: C is clear at the terminator, Z clear at a difference */
	mov ip, #4
1:
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	bne 9f
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	bne 9f
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	bne 9f
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	bne 9f
	subs ip, ip, #1
	bne 1b

	/* Bytes to a word boundary of the first string */
	tst r0, #3
	beq 2f
5:
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	bne 9f
	tst r0, #3
	bne 5b
2:
	eor r2, r0, r1
	tst r2, #3
	bne 3f
4:
	ldr r2, [r0], #4
	ldr r3, [r1], #4
	sub ip, r2, #0x01010101
	bic ip, ip, r2
	and ip, ip, #0x80808080
	eor r3, r3, r2
	orrs ip, ip, r3
	beq 4b
	rev ip, ip
	clz ip, ip
	add r0, r0, ip, lsr #3
	add r1, r1, ip, lsr #3
	ldrb r2, [r0, #-4]
	ldrb r3, [r1, #-4]
	subs r0, r2, r3
	bx lr

	/* Different phases: the byte loop to the end */
3:
	ldrb r2, [r0], #1
	ldrb r3, [r1], #1
	cmp r2, #1
	it cs
	cmpcs r2, r3
	beq 3b
9:
	subs r0, r2, r3
	bx lr
	.size strcmp, . - strcmp

	.section .note.GNU-stack, "", %progbits
