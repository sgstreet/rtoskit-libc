GITHUB_ACCOUNT ?= sgstreet

# arm-none-eabi GCC 16.2 (red-rocket-computing/cross-dev release v0.3.0). The target is the
# versioned driver, so changing the version installs the new toolchain on the next build.
COMPILER_VERSION ?= v0.3.0
COMPILER_GCC_VERSION := 16.2.0
COMPILER_ARCHIVE := gcc-arm-none-eabi-$(subst .,-,${COMPILER_GCC_VERSION}).tar.xz
COMPILER_PATH := ${PROJECT_ROOT}/tmp
COMPILER_URL := https://github.com/red-rocket-computing/cross-dev/releases/download/${COMPILER_VERSION}/${COMPILER_ARCHIVE}
COMPILER_TARGET := ${PROJECT_ROOT}/local/bin/arm-none-eabi-gcc-${COMPILER_GCC_VERSION}

SETUP_TARGETS := ${COMPILER_TARGET}
SETUP_PATHS := ${COMPILER_PATH}

include ${PROJECT_ROOT}/tools/makefiles/common.mk

${COMPILER_TARGET}:
	@echo "DOWNLOADING ${COMPILER_URL}"
	[ -d ${COMPILER_PATH} ] || mkdir -p ${COMPILER_PATH}
	cd ${COMPILER_PATH} && wget -nv ${COMPILER_URL}
	@echo "INSTALLING ${@}"
	$(RM) -r ${PROJECT_ROOT}/local
	mkdir -p ${PROJECT_ROOT}/local
	tar -C ${PROJECT_ROOT}/local --strip-components=1 -xf ${COMPILER_PATH}/${COMPILER_ARCHIVE}
	$(RM) -rf ${COMPILER_PATH}/${COMPILER_ARCHIVE}
	
setup-realclean:
	@echo "REMOVING ${SETUP_PATHS}"
	-$(RM) -rf ${SETUP_PATHS} ${PROJECT_ROOT}/local
.PHONY: setup-realclean

setup-tools: ${SETUP_TARGETS}
	@:
.PHONY: setup-tools

setup: setup-tools
	@:
.PHONY: setup

