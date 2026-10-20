#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#


# The generic functions, less those the hand-written kernels of this core provide (PLAN D69)
LIBC_STRING_OVERRIDES := $(foreach kernels,${LIBC_STRING_KERNELS},$(patsubst %.S,%.c,$(notdir $(wildcard ${SOURCE_DIR}/${kernels}/*.S))))
SRC := $(filter-out $(addprefix ${SOURCE_DIR}/,${LIBC_STRING_OVERRIDES}),$(wildcard ${SOURCE_DIR}/*.c))
