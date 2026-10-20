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
 * C23 7.26.4.2 for Thumb-1 (Armv6-M), written for size (PLAN D69): the byte
 * loop of the C version; the characters compare as unsigned char (ldrb
 * zero-extends, 7.26.4p1), and the difference of the first pair that differs
 * or ends the first string is the result.
 */
	.syntax unified
	.thumb
	.text
	.global strcmp
	.type strcmp, %function
strcmp:
	ldrb r2, [r0]
	ldrb r3, [r1]
	adds r0, #1
	adds r1, #1
	subs r2, r2, r3
	bne 1f
	cmp r3, #0
	bne strcmp
1:
	movs r0, r2
	bx lr
	.size strcmp, . - strcmp

	.section .note.GNU-stack, "", %progbits
