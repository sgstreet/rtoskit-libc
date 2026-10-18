#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# libc.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Shared fragment for modules that build library code (libc itself and the
# RTOS ports). Include it from the module's subdir.mk so it runs after the
# <arch>.mk toolchain file.
#
# Inputs:
#   LIBC_BASE    tree to select sources from (default ${PROJECT_ROOT}); the
#                generic sources live in ${LIBC_BASE}/src and the arch layers
#                in ${LIBC_BASE}/arch/<layer>/src
#   ARCH_LAYERS  ordered arch layers from <arch>.mk, most specific first
#
# For every source path relative to its root (e.g. string/memcpy), the first
# layer providing it wins and the generic file is used only when no layer
# does (PLAN D5, D17). Headers resolve the same way through the include path.

LIBC_BASE ?= ${PROJECT_ROOT}

LIBC_ROOTS := $(addprefix ${LIBC_BASE}/arch/,$(addsuffix /src,${ARCH_LAYERS})) ${LIBC_BASE}/src

# $(call libc-key,file,root): selection key of a file, its path under root without the extension
libc-key = $(basename $(patsubst $(2)/%,%,$(1)))

# $(call libc-find,root): every C and assembler source below root
libc-find = $(sort $(shell find $(1) -type f \( -name '*.c' -o -name '*.S' \) 2>/dev/null))

# $(call libc-select,roots): first source per key across roots in priority order
define libc-select
$(strip $(foreach root,$(1),$(foreach file,$(call libc-find,$(root)),\
	$(if $(__libc_key_$(call libc-key,$(file),$(root))),,\
		$(eval __libc_key_$(call libc-key,$(file),$(root)) := 1)$(file)))))
endef

# Evaluated once: libc-select records the keys it has already chosen
LIBC_SRC := $(call libc-select,${LIBC_ROOTS})
SRC += ${LIBC_SRC}

# project.mk's own rule compiles sources below the module directory without
# creating their object directories, so create every object directory up front
$(shell mkdir -p $(sort $(dir $(subst ${PROJECT_ROOT},${BUILD_ROOT},${LIBC_SRC}))))

# Library code sees its own internal headers before the public ones
CPPFLAGS += $(addprefix -I,${LIBC_ROOTS})
CFLAGS += -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -ftls-model=local-exec

# Sources outside the module directory compile into the matching build tree path
${BUILD_ROOT}/%.o: ${PROJECT_ROOT}/%.c ${EXTRA_OBJ_DEPS}
	@echo "COMPILING $<"
	mkdir -p $(@D)
	$(CC) ${CPPFLAGS} ${CFLAGS} -Wa,${LISTING_STYLE}="$@.lst" -MMD -MP -c -o $@ $<

${BUILD_ROOT}/%.o: ${PROJECT_ROOT}/%.S ${EXTRA_OBJ_DEPS}
	@echo "ASSEMBLING $<"
	mkdir -p $(@D)
	$(CC) ${CPPFLAGS} -x assembler-with-cpp ${ASFLAGS} -Wa,${LISTING_STYLE}="$@.lst" -MMD -MP -c -o $@ $<

# $(call libc-archive,objects): recreate an archive so dropped members never linger
define libc-archive
	@echo "ARCHIVING $@"
	${RM} $@
	$(AR) ${ARFLAGS} $@ $(1)
endef

# $(call libc-install,source,destination)
define libc-install
	@echo "INSTALLING $(2)"
	$(INSTALL) -m 644 -D $(1) $(2)
endef
