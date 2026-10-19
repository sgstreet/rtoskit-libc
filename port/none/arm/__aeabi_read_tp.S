/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_read_tp.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * void *__aeabi_read_tp(void): the thread pointer __rtos_none_set_tp stored.
 * The run-time ABI allows only r0 to change; Thumb-1 only, for armv6-m.
 */
	.syntax unified
	.thumb
	.text
	.global __aeabi_read_tp
	.type __aeabi_read_tp, %function
__aeabi_read_tp:
	ldr r0, =__rtos_none_tp
	ldr r0, [r0]
	bx lr
	.size __aeabi_read_tp, . - __aeabi_read_tp
	.ltorg

	.section .note.GNU-stack, "", %progbits
