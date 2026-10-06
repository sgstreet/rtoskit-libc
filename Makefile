.DEFAULT_GOAL := all
ifeq (${MAKECMDGOALS},)
MAKECMDGOALS := all
endif

BUILDS ?= debug release

export ARCH_CROSS ?= host
export BUILD_TYPE ?= all
export PROJECT_ROOT ?= ${CURDIR}
export OUTPUT_ROOT ?= ${PROJECT_ROOT}/build
export TOOLS_ROOT ?= ${PROJECT_ROOT}/tools
export PREFIX ?= ${PROJECT_ROOT}/images

ifeq (${V},)
SILENT=--silent
endif

MAKEFLAGS += ${SILENT}

ifeq (${BUILD_TYPE},all)

export INSTALL_ROOT := ${PREFIX}$(if ${BOARD_TYPE},/${BOARD_TYPE},)
export BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)

all:
	@for build_type in ${BUILDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BUILD_TYPE=$$build_type all; \
	done
.PHONY: all

clean:
	@for build_type in ${BUILDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BUILD_TYPE=$$build_type clean; \
	done
.PHONY: clean

else

export INSTALL_ROOT := ${PREFIX}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
export BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)

include ${TOOLS_ROOT}/makefiles/setup.mk
include ${TOOLS_ROOT}/makefiles/tree.mk

# Dependency ordering only active for full builds (all/clean/install)
# Individual module targets (make rtos, make test/unit/fault) are dep-free
ifneq ($(filter-out all target clean install,${MAKECMDGOALS}),)
target:
else
target: example
endif

endif

db:
	bear --append -- ${MAKE} all

distclean:
	@echo "DISTCLEAN ${PREFIX} ${OUTPUT_ROOT} ${PROJECT_ROOT}/compile_commands.json"
	-${RM} -r ${PREFIX} ${OUTPUT_ROOT} ${PROJECT_ROOT}/compile_commands.json

realclean: distclean
	@echo "REALCLEAN ${PROJECT_ROOT}/tmp ${PROJECT_ROOT}/local"
	-${RM} -r ${PROJECT_ROOT}/tmp ${PROJECT_ROOT}/local

info:
	@echo "ARCH_CROSS=${ARCH_CROSS}"
	@echo "CROSS_COMPILE=${CROSS_COMPILE}"
	@echo "BUILD_TYPE=${BUILD_TYPE}"
	@echo "PROJECT_ROOT=${PROJECT_ROOT}"
	@echo "OUTPUT_ROOT=${OUTPUT_ROOT}"
	@echo "TOOLS_ROOT=${TOOLS_ROOT}"
	@echo "PREFIX=${PREFIX}"
	@echo "BUILD_ROOT=${BUILD_ROOT}"
	@echo "INSTALL_ROOT=${INSTALL_ROOT}"
.PHONY: info

help:
	@echo "Targets:"
	@echo "  all                    Build everything (full ordered build)"
	@echo "  clean                  Clean all build artifacts"
	@echo "  install                Install all build artifacts"
	@echo "  db                     Generate compilation database (bear)"
	@echo "  setup                  Download and install host build tools"
	@echo "  distclean              Delete all build artifacts and output directories"
	@echo "  realclean              Delete all build artifacts, directories and downloaded tools"
	@echo "  info                   Display build configuration"
	@echo ""
	@echo "Direct module targets (dep-free, assumes prior full build):"
	@echo "  <module>               Build a top-level module, e.g. rtos, hardware, test"
	@echo "  <module>/<submodule>   Build a nested module, e.g. test/unit, scheduler/multicore"
	@echo "  test/unit/<name>       Build a single unit test, e.g. test/unit/fault"
	@echo ""
	@echo "Configuration variables:"
	@echo "  BOARD_TYPE=all         Which board(s): all (pi-pico2 + pico-can, both RP2350), or a single board"
	@echo "  BUILD_TYPE=debug       Build variant: debug, release, or all"
	@echo "  V=1                    Verbose output (show full compiler commands)"
	@echo "  PROJECT_ROOT=<path>    Override project root (required in worktrees)"
.PHONY: help

${V}.SILENT:
