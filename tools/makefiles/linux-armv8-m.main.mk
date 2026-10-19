#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# linux-armv8-m.main.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Linux process on armv8-m.main, run under qemu-user and bound to port/linux

export ARCH_CROSS := armv8-m.main
export RTOS_PORT := linux

# qemu-arm runs the M-profile code on its A-profile "max" CPU; see docs/research/arm-bring-up.md
export TARGET_RUNNER := qemu-arm -cpu max

# Load the ELF headers so the port finds the TLS segment through __ehdr_start
export BOARD_LDFLAGS := -Wl,-T,${PROJECT_ROOT}/port/linux/arm/headers.ld

# The allocator built into libc.a: nano (small, the default) or tlsf (constant time); PLAN D32
export LIBC_MALLOC ?= nano
