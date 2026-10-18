# Phase 3 — Performance optimization pass

Prerequisite: `phase-2-complete`. Targets: `armv6-m`, `armv7e-m`,
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

## P3.01 — Benchmark infrastructure
- [ ] Research existing libc benchmarks (glibc `benchtests`, the ARM optimized-routines bench, the bionic benchmarks, picolibc's) for workload ideas.
- [ ] `test/bench/`: a harness that is self-contained against our libc. Workloads: memory/string functions over a size × alignment sweep, `printf`/`snprintf` mixes, `strtod`/`strtol`, the `malloc` patterns, `qsort`, the libm throughput and latency per function, and `<threads.h>` mutex uncontended.
- [ ] `make bench` produces a machine-readable baseline per arch. Capture the Phase 2 baseline.

## P3.02 — Memory and string functions
- [ ] armv7e-m and v8-m.main: `LDRD`/`LDM`-based `memcpy`/`memset`/`memmove`, word-at-a-time `strlen`/`strcmp`/`strchr` using `UADD8`/`SEL` (DSP), and IT-block tuning. These are the `speed/` flavour files.
- [ ] armv6-m: unrolled Thumb-1 variants (no unaligned access; the `LDM`/`STM` limits).
- [ ] x86_64: SSE2 baseline versions (the reference flavour only).
- [ ] Research note per kernel. Exhaustive alignment/length/overlap tests plus libc-test.

## P3.03 — Conversions: printf/scanf and strtod/strtol
- [ ] Fast paths for common formats (`%d`, `%s`, `%x`) and a digit-pair table (in the speed flavour only).
- [ ] An Eisel–Lemire-style fast path for `strtod` (if P1.07 chose a slower core); table trade-offs per flavour.

## P3.04 — malloc and locking
- [ ] Fast paths for small size classes, and uncontended lock fast paths in the seam contract (an inline atomic check before calling `__rtos_*`).
- [ ] Measure with the allocator benchmarks; check fragmentation regressions.

## P3.05 — libm hot paths
- [ ] armv7e-m and v8-m.main: the single-precision functions use the FPU fully (`vsqrt.f32`, `vfma`), with table-driven reductions in the speed flavour.
- [ ] armv6-m: integer-only `float` implementations where they beat soft-float emulation.
- [ ] x86_64: use `sqrtsd`/`roundsd` (SSE4.1 only if the baseline ISA is raised; decide and record).

## P3.06 — Phase wrap-up
- [ ] Final perf report against the Phase 2 baseline, and against picolibc/newlib for context.
- [ ] Recheck the size budgets for both flavours. Full libc-test on all arches and flavours.
- [ ] Tag `phase-3-complete`.
