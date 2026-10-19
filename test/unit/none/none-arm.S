/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * none-arm.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */
	/* Thumb-1 for armv6-m; svc is emitted as .inst.n (docs/research/arm-bring-up.md) */
	.syntax unified
	.thumb
	.text
	.global _start
	.type _start, %function
_start:
	movs r0, #0
	mov lr, r0
	mov r0, sp
	bl none_test_start
	udf #0
	.size _start, . - _start

	/* void none_test_write(const void *buffer, size_t count): write(1, buffer, count) */
	.global none_test_write
	.type none_test_write, %function
none_test_write:
	push {r7, lr}
	mov r2, r1
	mov r1, r0
	movs r0, #1
	movs r7, #4
	.inst.n 0xdf00
	pop {r7, pc}
	.size none_test_write, . - none_test_write

	/* void none_test_exit(int status): exit_group(status) */
	.global none_test_exit
	.type none_test_exit, %function
none_test_exit:
	movs r7, #248
	.inst.n 0xdf00
	udf #0
	.size none_test_exit, . - none_test_exit

	.section .note.GNU-stack, "", %progbits
