GITHUB_ACCOUNT ?= sgstreet

# arm-none-eabi GCC 16.2 (red-rocket-computing/cross-dev release v0.3.0). The target is the
# versioned driver, so changing the version installs the new toolchain on the next build.
COMPILER_VERSION ?= v0.3.0
COMPILER_GCC_VERSION := 16.2.0
COMPILER_ARCHIVE := gcc-arm-none-eabi-$(subst .,-,${COMPILER_GCC_VERSION}).tar.xz
COMPILER_PATH := ${PROJECT_ROOT}/tmp
COMPILER_URL := https://github.com/red-rocket-computing/cross-dev/releases/download/${COMPILER_VERSION}/${COMPILER_ARCHIVE}
COMPILER_TARGET := ${PROJECT_ROOT}/local/bin/arm-none-eabi-gcc-${COMPILER_GCC_VERSION}

# The benchmark qemu (P3.01, PLAN D72): qemu-arm and qemu-x86_64 built with TCG plugin support,
# which the distribution packages leave out, and the instruction counting plugin. Only
# "make bench" needs them ("make setup-bench"); building qemu needs ninja, python3 and the
# glib development files. Installed after the compiler, whose install empties local/.
QEMU_VERSION := 8.2.2
QEMU_URL := https://download.qemu.org/qemu-${QEMU_VERSION}.tar.xz
QEMU_SOURCE := ${PROJECT_ROOT}/tmp/qemu-${QEMU_VERSION}
QEMU_ROOT := ${PROJECT_ROOT}/local/qemu-${QEMU_VERSION}
QEMU_TARGET := ${QEMU_ROOT}/bin/qemu-arm
BENCH_PLUGIN := ${QEMU_ROOT}/lib/bench-roi.so

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
	
${QEMU_TARGET}: | ${COMPILER_TARGET}
	@echo "BUILDING qemu ${QEMU_VERSION} with plugins"
	mkdir -p ${PROJECT_ROOT}/tmp
	cd ${PROJECT_ROOT}/tmp && wget -nv ${QEMU_URL}
	tar -C ${PROJECT_ROOT}/tmp -xf ${PROJECT_ROOT}/tmp/qemu-${QEMU_VERSION}.tar.xz
	cd ${QEMU_SOURCE} && ./configure --target-list=arm-linux-user,x86_64-linux-user --enable-plugins --disable-docs --disable-tools --disable-system > configure.log
	ninja -C ${QEMU_SOURCE}/build qemu-arm qemu-x86_64
	install -m 755 -D ${QEMU_SOURCE}/build/qemu-x86_64 ${QEMU_ROOT}/bin/qemu-x86_64
	install -m 644 -D ${QEMU_SOURCE}/include/qemu/qemu-plugin.h ${QEMU_ROOT}/include/qemu-plugin.h
	install -m 755 -D ${QEMU_SOURCE}/build/qemu-arm ${@}
	$(RM) -r ${QEMU_SOURCE} ${PROJECT_ROOT}/tmp/qemu-${QEMU_VERSION}.tar.xz

${BENCH_PLUGIN}: ${PROJECT_ROOT}/tools/qemu-plugins/bench-roi.c ${QEMU_TARGET}
	@echo "BUILDING ${@}"
	mkdir -p $(dir ${@})
	cc -std=gnu2x -O2 -Wall -Werror -shared -fPIC $(shell pkg-config --cflags glib-2.0) -I${QEMU_ROOT}/include -o ${@} ${<}

setup-bench: ${QEMU_TARGET} ${BENCH_PLUGIN}
	@:
.PHONY: setup-bench

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

