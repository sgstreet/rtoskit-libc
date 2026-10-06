#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# tree.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

where-am-i := ${CURDIR}/$(lastword $(subst $(lastword ${MAKEFILE_LIST}),,${MAKEFILE_LIST}))

# Shallow discovery (standard recursive builds via 'all')
SUBDIRS ?= $(subst ${CURDIR}/,,$(shell find ${CURDIR} -mindepth 2 -maxdepth 2 -name "*.mk" -and -not -name "subdir.mk" -printf "%h "))

# Deep discovery: all module directories at any depth
ALL_MODULES := $(subst ${CURDIR}/,,$(sort $(shell find ${CURDIR} -mindepth 2 \
    -name "*.mk" -not -name "subdir.mk" \
    -not -path "${CURDIR}/build/*" \
    -not -path "${CURDIR}/tools/*" \
    -not -path "${CURDIR}/.claude/*" \
    -printf "%h\n" | sort -u)))

# Normalize goal for sub-makes (project.mk only knows all/clean/install)
FORWARD_GOAL := $(if $(filter clean,${MAKECMDGOALS}),clean,$(if $(filter install,${MAKECMDGOALS}),install,all))

# Top-level subdir rules
${SUBDIRS}:
	@echo "ENTERING $@"
	+${MAKE} --no-print-directory -C $@ -f $@.mk ${FORWARD_GOAL}
.PHONY: ${SUBDIRS}

# Deep module targets (paths not already in SUBDIRS)
DEEP_MODULES := $(filter-out ${SUBDIRS},${ALL_MODULES})

define DEEP_MODULE_RULE
$(1):
	@echo "ENTERING $(1)"
	+$${MAKE} --no-print-directory -C $(1) -f $(notdir $(1)).mk $${FORWARD_GOAL}
.PHONY: $(1)
endef

$(foreach m,${DEEP_MODULES},$(eval $(call DEEP_MODULE_RULE,${m})))

target:

all: target

clean: target

install: target

