# Phase 3 — Performance optimization pass

Prerequisite: `phase-2-complete`. Targets: `armv6-m`, `armv7e-m`, `armv7e-m-dp`,
`armv8-m.main` and `x86_64`.

**Every step's definition of done (DoD)** is the Phase 1 DoD plus:

- The benchmark results are recorded in `docs/perf/<arch>.md` against the
  previous baseline. No benchmark regresses by more than noise.
- The `size` flavour stays within the Phase 2 budget. The `speed` flavour stays
  within the budget +N% locked in P2.07 (D12, pending Q3).

Measurement (pending Q5): on ARM, instruction counts from a qemu TCG plugin
(`libinsn`) and, if a board is attached, DWT `CYCCNT` on RP2040/RP2350. On
x86_64, wall clock with `rdtsc`, pinned, median of N runs.

---

## P3.01 — Benchmark infrastructure  `[x]`
- [x] Research existing libc benchmarks (glibc `benchtests`, the ARM optimized-routines bench, the bionic benchmarks, picolibc's) for workload ideas. *`docs/research/benchmarks.md`: glibc's throughput/latency split and size × alignment sweeps, optimized-routines' realistic mixes (our own generator, not their SPEC tables), Bionic's buffer alignments; picolibc and newlib have no suite. Measurement (Q5, D72): instruction counts from a qemu TCG plugin on every board, x86_64 included; `rdtsc` cycles reported only, since this shared VM moves them by ±40 %.*
- [x] `test/bench/`: a harness that is self-contained against our libc. Workloads: memory/string functions over a size × alignment sweep, `printf`/`snprintf` mixes, `strtod`/`strtol`, the `malloc` patterns, `qsort`, the libm throughput and latency per function, and `<threads.h>` mutex uncontended. *`test/bench/harness` (count and time modes, out-of-line region markers, an `ARCH_FAMILY` cycle counter) and eight programs, 804 cases: the string sweep (10 functions × 15 sizes × 4 offsets or offset pairs, plus 3 mixes), stdio (23), strto (21), malloc with `libc.a` and with `libc-tinymalloc.a` (9 each), qsort (8), 31 math functions in float and double as throughput and latency (124), threads (7). Built for every board and build type (`-Werror`), run only by the runner.*
- [x] `tools/bench-runner` produces a machine-readable baseline per arch. Capture the Phase 2 baseline. *`tools/scripts/bench-runner` (`run`, `diff`, `report`, `markdown --previous`) with the plugin `tools/qemu-plugins/bench-roi.c` and qemu 8.2.2 built with plugins by `make setup-bench` (Ubuntu's has none); `make bench` runs and diffs the release build of every board, `BENCH_UPDATE=1` records. The plugin agrees with an independent exec-trace count on all 30 regions checked, two runs are byte-identical, and fault injection trips the gate. Baselines `docs/perf/baseline-<arch>.json` and tables `docs/perf/<arch>.md` for the six boards. The library is unchanged: libc-test 219/219 on all 12 board × build configurations, unit tests pass, size diff and budget unchanged.*

## P3.02 — Memory and string functions

Split into four steps (one ISA family each); the first also creates the
speed flavour itself (D12, Q3, D73).

### P3.02a — The speed flavour; Thumb-2 `memcpy`/`memmove`/`memset`  `[x]`
- [x] The speed flavour as a build type (`BUILD_TYPE=speed`, in `BUILDS`, so `make check` runs libc-test and the unit tests on it); kernel directories per flavour with precedence; `make bench` measures both flavours. *D73, answering Q3: `tools/makefiles/speed.mk` is `-Os` plus `LIBC_FLAVOUR := speed` (a whole `-O2` library breaks the +10 % budget everywhere, `hello-printf-int` +35 %), with `LIBC_SPEED_CFLAGS` for hot modules; `LIBC_STRING_KERNELS_size`/`_speed` per arch, first directory wins; `bench-runner --build-type speed`, `docs/perf/baseline-<arch>-speed.json`.*
- [x] armv7e-m, armv7e-m-dp and v8-m.main: `LDRD`/`LDM`-based `memcpy`/`memset`/`memmove` and IT-block tuning. These are the `speed/` flavour files. *`libc/string/thumb2`: 32-byte `LDM`/`STM` blocks, unaligned LDRs for a misaligned source, a branch-skipped bit tail (branches beat IT blocks: every skipped conditional instruction still issues); LDM chosen over LDRD (1 + N cycles against 3 per 8 bytes on an M4, the same 64-bit beats on an M7). armv7e-m: `memcpy` 1024 B 1313 → 174 instructions aligned, 4124 → 441 misaligned, the small-copy mix 215 → 54, `memset` 1024 B 1054 → 177; 319 cases faster per core, none beyond noise slower. Trimmed to 180 + 218 + 108 bytes so the speed budget holds (tightest 2034 of 2039 bytes).*
- [x] Research note. Exhaustive alignment/length/overlap tests plus libc-test, on the speed flavour. *`docs/research/string-speed.md`. `test/unit/string_kernels` (every offset pair mod 8, lengths to 80 and 127–129, 255–257, every overlap) passes; 13 of 14 mutants killed, the survivor equivalent (memmove onto itself). From-scratch `make -j8 check` with the speed build type: libc-test 219/219 and every unit test on all 18 board × build configurations; the size flavour is unchanged (`size-report diff`, budget).*

### P3.02b — Thumb-2 word-at-a-time string functions
- [ ] armv7e-m, armv7e-m-dp and v8-m.main: word-at-a-time `strlen`/`strcmp`/`strchr` using `UADD8`/`SEL` (DSP). Armv8-M Mainline has the DSP extension only as an option: check what `-march=armv8-m.main` allows.
- [ ] Research note. Exhaustive tests plus libc-test.

### P3.02c — armv6-m speed variants
- [ ] armv6-m: unrolled Thumb-1 variants (no unaligned access; the `LDM`/`STM` limits).
- [ ] Research note. Exhaustive tests plus libc-test.

### P3.02d — x86_64
- [ ] x86_64: SSE2 baseline versions (the reference flavour only).
- [ ] Research note. Exhaustive tests plus libc-test.

## P3.03 — Conversions: printf/scanf and strtod/strtol
- [ ] Fast paths for common formats (`%d`, `%s`, `%x`) and a digit-pair table (in the speed flavour only).
- [ ] An Eisel–Lemire-style fast path for `strtod` (if P1.07 chose a slower core); table trade-offs per flavour.

## P3.04 — malloc and locking
- [ ] Fast paths for small size classes, and uncontended lock fast paths in the seam contract (an inline atomic check before calling `__rtos_*`).
- [ ] Measure with the allocator benchmarks; check fragmentation regressions.

## P3.05 — libm hot paths
- [ ] armv7e-m and v8-m.main: the single-precision functions use the FPU fully (`vsqrt.f32`, `vfma`), with table-driven reductions in the speed flavour.
- [ ] armv7e-m-dp: the double-precision functions use the DP unit fully (`vsqrt.f64`, `vfma.f64`, `vrint*.f64`). Re-tune the `double` algorithms for hardware FMA, where polynomial and error-compensation choices differ from soft-float. The `fpv5-dp` layer is shared, so a later Armv8.1-M DP target (Q10) can reuse it.
- [ ] Cortex-M7 specifics for armv7e-m-dp: the dual-issue pipeline and the 64-bit AXI/TCM path; `LDRD`/`VLDM` for `memcpy`. Measure with the DWT cycle counter on hardware if available (Q5).
- [ ] armv6-m: integer-only `float` implementations where they beat soft-float emulation.
- [ ] x86_64: use `sqrtsd`/`roundsd` (SSE4.1 only if the baseline ISA is raised; decide and record).

## P3.06 — Phase wrap-up
- [ ] Final perf report against the Phase 2 baseline, and against picolibc/newlib for context.
- [ ] Recheck the size budgets for both flavours. Full libc-test on all arches and flavours.
- [ ] Tag `phase-3-complete`.
