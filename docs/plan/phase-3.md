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

## P3.02 — Memory and string functions  `[x]`

Split into four steps (one ISA family each); the first also creates the
speed flavour itself (D12, Q3, D73).

### P3.02a — The speed flavour; Thumb-2 `memcpy`/`memmove`/`memset`  `[x]`
- [x] The speed flavour as a build type (`BUILD_TYPE=speed`, in `BUILDS`, so `make check` runs libc-test and the unit tests on it); kernel directories per flavour with precedence; `make bench` measures both flavours. *D73, answering Q3: `tools/makefiles/speed.mk` is `-Os` plus `LIBC_FLAVOUR := speed` (a whole `-O2` library breaks the +10 % budget everywhere, `hello-printf-int` +35 %), with `LIBC_SPEED_CFLAGS` for hot modules; `LIBC_STRING_KERNELS_size`/`_speed` per arch, first directory wins; `bench-runner --build-type speed`, `docs/perf/baseline-<arch>-speed.json`.*
- [x] armv7e-m, armv7e-m-dp and v8-m.main: `LDRD`/`LDM`-based `memcpy`/`memset`/`memmove` and IT-block tuning. These are the `speed/` flavour files. *`libc/string/thumb2`: 32-byte `LDM`/`STM` blocks, unaligned LDRs for a misaligned source, a branch-skipped bit tail (branches beat IT blocks: every skipped conditional instruction still issues); LDM chosen over LDRD (1 + N cycles against 3 per 8 bytes on an M4, the same 64-bit beats on an M7). armv7e-m: `memcpy` 1024 B 1313 → 174 instructions aligned, 4124 → 441 misaligned, the small-copy mix 215 → 54, `memset` 1024 B 1054 → 177; 319 cases faster per core, none beyond noise slower. Trimmed to 180 + 218 + 108 bytes so the speed budget holds (tightest 2034 of 2039 bytes).*
- [x] Research note. Exhaustive alignment/length/overlap tests plus libc-test, on the speed flavour. *`docs/research/string-speed.md`. `test/unit/string_kernels` (every offset pair mod 8, lengths to 80 and 127–129, 255–257, every overlap) passes; 13 of 14 mutants killed, the survivor equivalent (memmove onto itself). From-scratch `make -j8 check` with the speed build type: libc-test 219/219 and every unit test on all 18 board × build configurations; the size flavour is unchanged (`size-report diff`, budget).*

### P3.02b — Thumb-2 word-at-a-time string functions  `[x]`
- [x] armv7e-m, armv7e-m-dp and v8-m.main: word-at-a-time `strlen`/`strcmp`/`strchr` using `UADD8`/`SEL` (DSP). Armv8-M Mainline has the DSP extension only as an option: check what `-march=armv8-m.main` allows. *D74. All three builds have DSP (armv8-m.main is `+dsp`). `strchr` uses `UADD8`/`SEL`. `strcmp` uses a 16-byte unrolled byte loop (6 instructions a byte against 8), then words with the immediate-constant zero test: DSP needed a sixth register whose push regressed short strings. armv7e-m: `strcmp` 1024 B 8220 → 2160, `strchr` 2874 → 2083; 121 cases faster per core, none slower than either baseline. `strlen` stays Thumb-1: the regression-free LDRD version is 10 % faster for +56 bytes, which the `malloc-heavy` speed budget (5–13 bytes left) cannot take (Q16).*
- [x] Research note. Exhaustive tests plus libc-test. *`docs/research/string-speed.md` (P3.02b section, four designs measured). `test/unit/string_kernels` gains `strchr`: every offset, length and position, aliasing bytes, the terminator, `int` beyond a byte. Mutants: 9 of 10 assembling ones killed, the survivor equivalent in tests (a narrower phase test whose unaligned loads still work on these cores). From-scratch `make -j8 check`: libc-test 219/219 and all unit tests on the 18 configurations; size flavour unchanged, both budgets hold.*

### P3.02c — armv6-m speed variants  `[x]`
- [x] armv6-m: unrolled Thumb-1 variants (no unaligned access; the `LDM`/`STM` limits). *D75, `libc/string/thumb1-speed` on armv6-m and armv6s-m: 16-byte `LDM`/`STM` blocks for equal phases at any size, shift-merged aligned words for different phases from 24 bytes (13 instructions per 8 bytes against 32), the size flavour's indexed loops kept for short and backward moves, a 3-instruction `memset` word loop, a 7-instruction `strcmp` byte loop. armv6-m: `memcpy` 1024 B 1314 → 292 aligned, 4125 → 1867 misaligned, the small-copy mix 223 → 136, `memset` 1024 B 1054 → 169; 260 cases faster, none slower (the first version had 49 regressions, fixed). `strlen` stays the size flavour's.*
- [x] Research note. Exhaustive tests plus libc-test. *`docs/research/string-speed.md` (P3.02c section). `test/unit/string_kernels` passes on the armv6-m/armv6s-m speed builds (qemu faults unaligned LDM/STM); all 14 mutants killed. From-scratch `make -j8 check`: libc-test 219/219 and all unit tests on the 18 configurations; the size flavour is unchanged and both budgets hold.*

### P3.02d — x86_64  `[x]`
- [x] x86_64: SSE2 baseline versions (the reference flavour only). *D76, `libc/string/x86_64` in the speed flavour: overlapping head/tail moves up to 32 bytes and an aligned 32-byte loop above (all loads before stores, so `memmove` reuses it forwards), aligned-block `strlen`/`strchr` (no page crossing), an 8-pair byte prologue and page-checked unaligned blocks with a `PMINUB` mask in `strcmp`. `memcpy` 1024 B 1463 → 296 aligned, 6184 → 303 misaligned, the small-copy mix 338 → 40, `strcmp` 1024 B 7203 → 1288; 483 cases faster, none slower (byte prologues and early returns removed 26 tiny-case regressions of the first version).*
- [x] Research note. Exhaustive tests plus libc-test. *`docs/research/string-speed.md` (P3.02d section). `test/unit/string_kernels` now covers offsets modulo 16, strings across a page boundary and zero bytes before a string, on every board and build type; 14 of 18 mutants killed, 3 survivors equivalent and 1 (the `strcmp` page check) observable only with a guard page. From-scratch `make -j8 check`: libc-test 219/219 and all unit tests on the 18 configurations; the size flavour is unchanged and both budgets hold.*

## P3.03 — Conversions: printf/scanf and strtod/strtol

Split in two (integers, then floating point).

### P3.03a — Integer conversions  `[x]`
- [x] `strtol` and relatives: no 64-bit division per call (the P3.01 baseline: `strtol("42")` costs 949 instructions on armv7e-m, mostly `__libc_udivmod64` for the overflow cutoff); both flavours, as long as the size flavour does not grow.
  *Speed flavour only (D77): the division-free version grew the size flavour's `__libc_strtoint` by 16–19 %. It accumulates in 32 bits while no digit can overflow, then uses `ckd_mul`/`ckd_add`; `strtol("42")` 949 → 135 on armv7e-m, 914 → 170 on armv6-m.*
- [x] Fast paths for common formats (`%d`, `%s`, `%x`) and a digit-pair table (in the speed flavour only), through a per-module `speed/` directory that overrides generic files (D12).
  *`libc/stdio/speed` and `libc/stdlib/speed` (D77). 64-bit decimals go in 10^9 chunks with computed digit pairs (a 200-byte table broke the speed budget); hex and octal shift 32 bits once the value fits; the buffer sink copies with `memcpy` from 16 bytes. `%lld` 7636 → 1353 on armv7e-m; 32-bit `%d` is unchanged (already one `UMULL` a digit), and on Armv6-M left for P3.06.*
- [x] Research note. Tests plus libc-test; size and speed budgets.
  *`docs/research/conversions-speed.md`; `test/unit/conversions` (strto* in every base around the limits against a reference parse; 64-bit printf), 11 mutants killed; size budget unchanged, speed budget holds.*

### P3.03b — Floating-point conversions  `[x]`
- [x] An Eisel–Lemire-style fast path for `strtod` (if P1.07 chose a slower core); table trade-offs per flavour.
  *Speed flavour only (D78): a middle path between Clinger and Simple Decimal Conversion, with 5^q rebuilt from two tables (624 bytes rather than Eisel–Lemire's 10 KiB) and an explicit error bound, in every rounding direction; Clinger's guard no longer divides and digits are tested in line. armv7e-m `DBL_MIN` 86186 → 1439, `6.02e300` 170760 → 1314; armv6-m `DBL_MIN` 1839969 → 3853. The size flavour keeps D38's small code.*
- [x] Research note. Tests plus libc-test; size and speed budgets.
  *`docs/research/conversions-speed.md`; `test/unit/strtod` gains short random inputs checked exactly in every direction and model-found fixed cases; 10 of 12 mutants killed, 2 equivalent; budgets hold, no benchmark case slower.*

## P3.04 — malloc and locking

Split in two (locks, then the allocators).

### P3.04a — Uncontended lock fast path  `[x]`
- [x] Uncontended lock fast paths in the seam contract: an inline atomic check before calling `__rtos_*`, in both ports.
  *D79: the lock word (owner token, slow bit) is now part of `rtos-port.h`, with a port-defined thread-local token. The Linux port's functions start with that compare-and-swap and keep the slow path for recursion and contention; `port/none` keeps no-ops. The in-line check in libc measured 92.5 → 67.5 instructions per malloc + free on armv7e-m, but breaks the speed budget by up to 200 bytes per image. The contract allows it, and libc doesn't use it yet (Q16).*
- [x] Research note; tests (recursion, contention, every lock user); measure with the allocator, stdio and thread benchmarks; size and speed budgets.
  *`docs/research/locks.md`; `test/unit/locks` (word states, nesting through both paths, tokens, four threads contending); `test/unit/threads` covers malloc and stdio. Both flavours, malloc + free: armv7e-m 107.5 → 92.5, x86-64 107 → 84, armv6-m 233 → 224; no case slower. Both budgets hold. Images shrink; the Armv6-M acquire frame grows by 8 bytes, justified in `docs/size`.*

### P3.04b — Allocator fast paths  `[x]`
- [x] Fast paths for small size classes.
  *Measured and rejected (D80). Exact-size bins in front of nano's first fit: twelve, with consolidation, and three. All lose more on mixed sizes than they win on one: `fixed/16` −12 %, but `batch/fifo` +11 % and `realloc/grow` +48–223 %, through unmerged chunks. TLSF was 70–170 % slower, and nano at `-O2` gained nothing. The lock fast path (P3.04a) was the one that paid.*
- [x] Measure with the allocator benchmarks; check fragmentation regressions.
  *`docs/research/malloc-speed.md` has every variant's numbers per board. The allocator is unchanged, so there is no fragmentation regression; one pinned small chunk was enough to break in-place `realloc` growth, which is the evidence against deferred coalescing here.*

## P3.05 — libm hot paths

Split by target (the speed baseline after P3.04: `sinf` 19k instructions on Armv6-M, 690 on FPv4/FPv5-SP, 225 on FPv5-DP).

### P3.05a — x86_64  `[x]`
- [x] x86_64: use `sqrtsd`/`roundsd` (SSE4.1 only if the baseline ISA is raised; decide and record).
  *D81: the baseline stays SSE2 (x86_64 is the test host and perf reference, which must run on any x86-64). `sqrt`/`sqrtf` already use `sqrtsd`/`sqrtss` (the `x86_64` libm layer). The SSE2 rounding alternative, a `cvttsd2si` round trip, raises inexact, which `floor`/`ceil`/`trunc`/`round` must not and libc-test checks; saving and restoring MXCSR costs more than the 30 instructions of the generic bit manipulation. So the rounding stays generic.*

### P3.05b — Single precision on FPv4-SP and FPv5-SP  `[x]`
- [x] armv7e-m and v8-m.main: the single-precision functions use the FPU fully (`vsqrt.f32`, `vfma`), with table-driven reductions in the speed flavour (`sinf`/`cosf`/`tanf` 690–1030 instructions against 225–325 on FPv5-DP; `erff`/`erfcf`, `tgammaf`, `lgammaf`).
  *D82: the float-float exact product is one `VFMA.F32` (`libc/internal/fpu/<fpu>/libc-fpu.h`), in both flavours, with bit-identical results. `erff` 2539 → 1384, `expf` 228 → 161, `sinf` 694 → 576, 54 cases faster; `libc.a` −4.3 KiB. `vsqrt.f32` was already used (P1.13). Reductions: the integer Payne–Hanek one is over half of `sinf`, but a float Cody–Waite one cannot keep the 2^-40 budget near the smallest |d| (2^-30, counted over every float), so it stays (research note).*
- [x] Research note; tests; size and speed budgets.
  *`docs/research/libm-fma.md`; `test/unit/softfp` checks the product exact on every board; the float accuracy tests are unchanged and pass; both budgets hold, and `libc.a`, `math-float` and `math-mix` shrink.*

### P3.05c — Double precision on FPv5-DP  `[x]`
- [x] armv7e-m-dp: the double-precision functions use the DP unit fully (`vsqrt.f64`, `vfma.f64`, `vrint*.f64`). Re-tune the `double` algorithms for hardware FMA, where polynomial and error-compensation choices differ from soft-float. The `fpv5-dp` layer is shared, so a later Armv8.1-M DP target (Q10) can reuse it.
  *D83: the double-double exact product is one `VFMA.F64`, with bit-identical results. `vsqrt.f64` and `vrint*.f64` were already in the `fpv5-dp` layer (P1.13, P2.05). `asin` 337 → 221, `tan` 307 → 208, `sin` 219 → 166 instructions; 102 cases faster, none slower; `libc.a` −3.4 KiB. The polynomials are not re-tuned: the error budget lives in the double-double steps, and fewer terms would need a new error analysis per core (research note).*
- [x] Research note; tests; size and speed budgets.
  *`docs/research/libm-fma.md` (P3.05c); `test/unit/softfp` compares the board's float and double products with Dekker's, bit for bit. Both budgets hold; `asinh`'s frame grows by 8 bytes (justified in `docs/size/armv7e-m-dp.md`).*

### P3.05d — Cortex-M7 specifics  `[x]`
- [x] Cortex-M7 specifics for armv7e-m-dp: the dual-issue pipeline and the 64-bit AXI/TCM path; `LDRD`/`VLDM` for `memcpy`. Measure with the DWT cycle counter on hardware if available (Q5).
  *D84: no hardware is attached, and qemu's instruction count cannot see dual issue or bus width. `memcpy` already moves 32 bytes per `LDM`/`STM` pair, more than `LDRD` per instruction and as much as `VLDM` without touching FPU registers. No M7 variant until DWT cycle counts exist (Q5 stays open).*

### P3.05e — Armv6-M integer float operations  `[x]`
Split (the step named four function families; each fixed-point rewrite needs its own error analysis): P3.05e is the soft-float operations every float function calls, and `sqrtf`; P3.05f–h are the fixed-point functions.
- [x] armv6-m: integer-only `float` implementations where they beat soft-float emulation (`sqrtf` 2k instructions); the multiplication and division helpers.
  *D85: `__aeabi_fmul` (four 12-bit half products) and `__aeabi_fdiv` (restoring division) in integers in the `nofp` layer, with one shared rounding (`softfloat.h`); `sqrtf` by a 32-bit digit recurrence. `sqrtf` 2036 → 461, `hypotf` 8371 → 6293, `erff` 151747 → 141024, `expf` 11458 → 11034; 62 cases faster, none slower. The helpers are 340 and 296 bytes against libgcc's 504 and 512; `math-float` −604 bytes. An integer `__aeabi_fadd` was exact but slower than libgcc's (51 cases slower), so it was not taken.*
- [x] Research note; tests; size and speed budgets.
  *`docs/research/softfloat.md`; `test/unit/softfp` checks `fmul`, `fdiv` and `sqrtf` against double on armv6-m, armv6s-m and x86-64, with operands that reach every guard and sticky position (mutants caught); on the host, all 2^32 `sqrtf` inputs and 3 × 10^8 operation pairs match the hardware. Both budgets hold; the 24-byte stack growth of `sinf`, `expf` and `powf` is justified in `docs/size/armv6-m.md`.*

### P3.05f — Armv6-M fixed-point exponentials and logarithms  `[x]`
- [x] armv6-m: `expf`/`exp2f`/`logf`/`log2f` (11–13k instructions) in fixed point, correctly rounded as the `sp-nearest` layer is (D57); research note with the error analysis; tests; both budgets.
  *D86: speed flavour only (`libc/math/nofp/speed`), a 32-bit fixed-point fast path with Ziv's rounding test before the float-float code. `exp2f` 9269 → 316, `expf` 11034 → 375, `log2f` 10547 → 583, `logf` 12367 → 2210. The error bounds hold over every float argument (core code run on the host, `docs/research/libm-fixed.md`). New `test/unit/libm_dense` (dense sweeps where the error peaks) catches a fast path without the rounding test, which `float_accuracy` missed for the logarithms. Size flavour unchanged; speed budget holds (`math-float` 10520 of 10604).*

### P3.05g — Armv6-M fixed-point sine and cosine  `[x]`
- [x] armv6-m: `sinf`/`cosf`/`tanf` (18–36k instructions) in fixed point after the integer reduction (D60); research note; tests; both budgets.
  *D87: speed flavour, `__libc_sinf_fixed` after the existing reduction, with Ziv's test before the float-float code. `sinf` 18350 → 4729, `cosf` 18281 → 3742, `tanf` 36223 → 5140. The bounds hold over every argument (host). `math-float` went over the speed budget by 856 bytes; by the user's choice the speed flavour's math images may grow by +20 % (`budget.json`, `program_margins_percent`). `test/unit/libm_dense` gains trig sweeps and 46 arguments that only a correct error test rounds right.*

### P3.05h — Armv6-M fixed-point error function  `[x]`
- [x] armv6-m: `erff`/`erfcf` (120–142k instructions) in fixed point; research note; tests; both budgets.
  *D88: speed flavour, Taylor steps from ODE recurrences (erf below 2, erfc·e^(x²) from 2 to 9.25) with e^(-x²) from the exp2 core, and Ziv's test before the float-float code. `erff` 141024 → 2967, `erfcf` 119604 → 2874. The bound holds over every argument (host); the exhaustive check found and fixed three precision losses on the way. `test/unit/libm_dense` gains erf/erfc sweeps and 32 arguments that only a correct error test rounds right. No size image links the error functions; both budgets hold.*

## P3.06 — Phase wrap-up  `[x]`
- [x] Final perf report against the Phase 2 baseline, and against picolibc/newlib for context.
  *`docs/perf/phase-3.md` (D89): speed flavour against phase 2, string 0.52–0.89, strto 0.13–0.26, malloc 0.77–0.93, math 0.65–0.87 (geometric means of instruction counts). `bench-runner compare` builds the same benchmarks against picolibc 1.8.6 and newlib-nano 4.4.0 (shims in `tools/bench-compare`). rtoskit is faster in string, strto and (against picolibc) malloc, and slower in math (correct rounding) and qsort (Q17).*
- [x] Recheck the size budgets for both flavours. Full libc-test on all arches and flavours.
  *Both budgets hold on every board. The size flavour is 0.1–8.9 % below the phase 2 lock in every image, and the speed flavour is within +8.8 % (+18.9 % for Armv6-M `math-float`, D87). libc-test 219/219 and every unit test pass in all 18 configurations.*
- [x] Tag `phase-3-complete`.
  *Pushed by the maintainer on 62a4cdb (P3.06), then moved to 23ff075 so the phase includes P3.07.*

## P3.07 — Speed-flavour qsort (Q17)  `[x]`
- [x] `qsort` in the speed flavour: introsort (median-of-three quicksort, insertion sort for small ranges, heapsort past a depth limit), no recursion and a bounded stack; the size flavour keeps its heapsort. Research note; tests with adversarial inputs and a comparison-count bound; both budgets; benchmarks against the baseline and against picolibc/newlib-nano.
  *D90: a run check (n − 1 comparisons for an input in order, all equal or strictly reversed), then an introsort: Hoare partition, median of three or a ninther, insertion sort at 8 or fewer, heapsort past 2·log2 n; iterative, with pointer arithmetic. Speed flavour, armv7e-m: random 640 → 274 (picolibc 278, newlib-nano 277), strings 585 → 303, ordered inputs 27–36; 7 cases faster on every board, none slower. `test/unit/stdlib` compares the results with an insertion sort, element for element, and bounds the comparisons, including McIlroy's adversary (a mutant without the depth limit fails). No reference program links `qsort`; both budgets hold.*
