#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Oct 22, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# The soft-dp layer: compiled into the test and checked against the hardware on x86_64, its assembly checked
# against the C models on the cores that link it, nothing elsewhere (PLAN D99)
SOFTDP_LAYER := $(if $(filter x86_64,${ARCH_FAMILY}),direct.c,$(if $(filter soft-dp,${LIBC_MATH_LAYERS}),assembly.c,layer_none.c))
SRC := ${SOURCE_DIR}/softdp.c ${SOURCE_DIR}/${SOFTDP_LAYER}
SRC += $(if $(filter layer_none.c,${SOFTDP_LAYER}),,${SOURCE_DIR}/reference_add.c ${SOURCE_DIR}/reference_multiply.c)
