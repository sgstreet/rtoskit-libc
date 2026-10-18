.DEFAULT_GOAL := all
ifeq (${MAKECMDGOALS},)
MAKECMDGOALS := all
endif

BOARDS ?= linux-x86_64
BUILDS ?= debug release

export BOARD_TYPE ?= all
export BUILD_TYPE ?= all
export PROJECT_ROOT ?= ${CURDIR}
export OUTPUT_ROOT ?= ${PROJECT_ROOT}/build
export TOOLS_ROOT ?= ${PROJECT_ROOT}/tools
export PREFIX ?= ${PROJECT_ROOT}/images

ifeq (${V},)
SILENT=--silent
endif

MAKEFLAGS += ${SILENT}

ifeq (${BOARD_TYPE},all)

# Stop at the first board that fails instead of reporting only the last one
all:
	@for board in ${BOARDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BOARD_TYPE=$$board all || exit $$?; \
	done
.PHONY: all

clean:
	@for board in ${BOARDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BOARD_TYPE=$$board clean || exit $$?; \
	done
.PHONY: clean

check:
	@for board in ${BOARDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BOARD_TYPE=$$board check || exit $$?; \
	done
.PHONY: check

else ifeq (${BUILD_TYPE},all)

export INSTALL_ROOT := ${PREFIX}$(if ${BOARD_TYPE},/${BOARD_TYPE},)
export BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)

all:
	@for build_type in ${BUILDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BUILD_TYPE=$$build_type all || exit $$?; \
	done
.PHONY: all

clean:
	@for build_type in ${BUILDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BUILD_TYPE=$$build_type clean || exit $$?; \
	done
.PHONY: clean

check:
	@for build_type in ${BUILDS}; do \
		${MAKE} --no-print-directory -f ${PROJECT_ROOT}/Makefile BUILD_TYPE=$$build_type check || exit $$?; \
	done
.PHONY: check

else

export INSTALL_ROOT := ${PREFIX}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
export BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)

# Concurrent "install -D" calls can race creating the same parent directory,
# so create the install root once, serially, for goals that install anything.
NO_INSTALL_ROOT_GOALS := clean distclean realclean info help setup
ifneq ($(filter-out ${NO_INSTALL_ROOT_GOALS},$(or ${MAKECMDGOALS},all)),)
$(if $(shell mkdir -p '${INSTALL_ROOT}' 2>&1),$(error cannot create ${INSTALL_ROOT}))
endif

include ${TOOLS_ROOT}/makefiles/setup.mk
include ${TOOLS_ROOT}/makefiles/tree.mk

# Dependency ordering only active for full builds (all/clean/install)
# Individual module targets (make libc, make test/unit/smoke) are dep-free
ifneq ($(filter-out all target clean install check,${MAKECMDGOALS}),)
target:
else
example test: libc port
target: example test
endif

# Build, then run every unit test; a test directory may provide <name>.run to drive its binary
check: target
	${PROJECT_ROOT}/test/unit/run-unit ${BUILD_ROOT}/test/unit
.PHONY: check

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
	@echo "RTOS_PORT=${RTOS_PORT}"
	@echo "CROSS_COMPILE=${CROSS_COMPILE}"
	@echo "BOARD_TYPE=${BOARD_TYPE}"
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
	@echo "  check                  Build and run the tests"
	@echo "  db                     Generate compilation database (bear)"
	@echo "  setup                  Download and install host build tools"
	@echo "  distclean              Delete all build artifacts and output directories"
	@echo "  realclean              Delete all build artifacts, directories and downloaded tools"
	@echo "  info                   Display build configuration"
	@echo ""
	@echo "Direct module targets (dep-free, assumes prior full build; set BOARD_TYPE and BUILD_TYPE):"
	@echo "  <module>               Build a top-level module, e.g. libc, port, test"
	@echo "  <module>/<submodule>   Build a nested module, e.g. libc/string, port/linux"
	@echo "  test/unit/<name>       Build a single unit test, e.g. test/unit/smoke"
	@echo ""
	@echo "Configuration variables:"
	@echo "  BOARD_TYPE=all         Which board(s): all (${BOARDS}), or a single board"
	@echo "  BUILD_TYPE=debug       Build variant: debug, release, or all"
	@echo "  V=1                    Verbose output (show full compiler commands)"
	@echo "  PROJECT_ROOT=<path>    Override project root (required in worktrees)"
.PHONY: help

${V}.SILENT:
