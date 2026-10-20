/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bench-roi.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * A QEMU TCG plugin that counts the guest instructions executed between two
 * marker addresses (P3.01, docs/research/benchmarks.md). The harness in
 * test/bench calls bench_roi_begin() and bench_roi_end() around each measured
 * region; tools/scripts/bench-runner passes their addresses:
 *
 *   qemu-arm -plugin bench-roi.so,begin=0x8100,end=0x8120 -d plugin program.elf
 *
 * The addresses are those of the first instructions (for Thumb, the symbol
 * values with bit 0 cleared).
 *
 * Every executed instruction adds one to a counter through an inline
 * operation. Reaching the first instruction of the begin marker snapshots the
 * counter; reaching the end marker prints "bench-roi <count>" on QEMU's log
 * (stderr), so the n-th line belongs to the n-th region. The count includes
 * the few instructions of the markers themselves, which the harness measures
 * with an empty region and the runner subtracts. The benchmarks are single
 * threaded, so one counter shared by every vCPU is exact.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static uint64_t executed;
static uint64_t region_start;
static uint64_t begin_address = UINT64_MAX;
static uint64_t end_address = UINT64_MAX;

static void on_begin(unsigned int vcpu_index, void *userdata)
{
	region_start = executed;
}

static void on_end(unsigned int vcpu_index, void *userdata)
{
	char line[64];

	snprintf(line, sizeof(line), "bench-roi %" PRIu64 "\n", executed - region_start);
	qemu_plugin_outs(line);
}

static void on_translate(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
	size_t count = qemu_plugin_tb_n_insns(tb);

	for (size_t i = 0; i < count; ++i) {
		struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
		uint64_t address = qemu_plugin_insn_vaddr(insn);

		/* The snapshot callbacks go first, so a marker's own instruction counts after it */
		if (address == begin_address)
			qemu_plugin_register_vcpu_insn_exec_cb(insn, on_begin, QEMU_PLUGIN_CB_NO_REGS, nullptr);
		if (address == end_address)
			qemu_plugin_register_vcpu_insn_exec_cb(insn, on_end, QEMU_PLUGIN_CB_NO_REGS, nullptr);
		qemu_plugin_register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64, &executed, 1);
	}
}

static int parse_address(const char *text, uint64_t *address)
{
	char *end;

	*address = strtoull(text, &end, 0);
	return *end == '\0' ? 0 : -1;
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc, char **argv)
{
	for (int i = 0; i < argc; ++i) {
		int status = -1;

		if (strncmp(argv[i], "begin=", 6) == 0)
			status = parse_address(argv[i] + 6, &begin_address);
		else if (strncmp(argv[i], "end=", 4) == 0)
			status = parse_address(argv[i] + 4, &end_address);
		if (status != 0) {
			fprintf(stderr, "bench-roi: bad argument '%s'\n", argv[i]);
			return -1;
		}
	}
	qemu_plugin_register_vcpu_tb_trans_cb(id, on_translate);
	return 0;
}
