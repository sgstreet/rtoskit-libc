/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setjmp.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.13.1.1 for Armv8-M Mainline with FPv5-SP (AAPCS 5.1.1 and 5.1.2.1): save the callee-saved
 * r4-r11, the stack pointer, the return address and the callee-saved VFP
 * registers d8-d15 (the same storage as s16-s31, so this serves single- and
 * double-precision FPUs alike, PLAN D15). The floating-point status and
 * control register is not saved (C23 7.13.2.1 does not require it).
 * jmp_buf layout: r4-r11, sp, lr, d8-d15.
 */
	.syntax unified
	.thumb
	.text
	.global setjmp
	.type setjmp, %function
setjmp:
	mov ip, sp
	stmia r0!, {r4-r11, ip, lr}
	vstmia r0, {d8-d15}
	movs r0, #0
	bx lr
	.size setjmp, . - setjmp

	.section .note.GNU-stack, "", %progbits
