# Phase 1 — Compliant implementation

Read [PLAN.md](PLAN.md) first. Each step below fits in one session. Do the steps
in order unless a step says otherwise.

**Every step's definition of done (DoD) includes all of the following:**

1. Research notes exist in `docs/research/` for every non-trivial algorithm in
   the step (PLAN D2), and the code is clean-room (D1).
2. `make` builds the `debug` and `release` builds for every enabled arch with
   `-Werror`.
3. `make check` passes on every enabled arch with no PASS→FAIL regressions. The
   newly passing tests are recorded in `test/libc-test/expected/<arch>.txt`, and
   the `pending:` entries in `test/libc-test/manifest.txt` belong only to later
   steps.
4. Our own unit tests cover what libc-test does not cover.
5. No new `#if`/`#ifdef` beyond the allowed list in CLAUDE.md.
6. This file's checkboxes are updated, any new decisions are appended to
   PLAN.md, and the work is committed.

Enabled arches: `x86_64` from P1.01, and all three ARM targets from P1.06 on.

---

## P1.01 — Toolchain, layout, build integration, Linux port bootstrap
- [ ] Install/verify GCC 14 (`gcc-14`) and Clang 19 (if used), qemu-user, and `arm-none-eabi` (cross toolchain; needed from P1.06). Record the versions in `docs/toolchain.md`.
- [ ] Add a SessionStart hook (`.claude/`) that installs the toolchain packages in cloud sessions.
- [ ] Add `tools/makefiles/x86_64.mk`: gcc-14, `-std=c23 -nostdinc -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -ftls-model=local-exec -mlong-double-64` (D7, pending Q1), static link with `-nostdlib`, libgcc.
- [ ] Add `tools/makefiles/libc.mk`: include paths (`arch/${ARCH_FAMILY}/include`, i.e. one header set per target triple per D15; `port/${PORT}/include`; `include`), the generic-vs-arch file selection (D5), and `install`/`install-headers` targets that external callers can drive (D16).
- [ ] Change the top-level `Makefile` default to `ARCH_CROSS ?= x86_64`, add `PORT ?= linux`, and order modules as `libc → test`. Keep `example` building against the new libc.
- [ ] Create the skeleton: `include/`, `src/internal/`, `arch/x86_64/`, `port/linux/`, `port/none/`, `libc/` (produces `libc.a`), and `test/unit/`.
- [ ] Draft `include/rtos/rtos-port.h` (seam v0: exit, abort, write, read, clock, TLS hooks).
- [ ] Linux port: x86_64 `_start` (crt1), raw syscall helpers, TLS block setup via `arch_prctl(ARCH_SET_FS)` and `__libc_tls_init` (D10), argc/argv/envp, then `main` → `exit`.
- [ ] Freestanding headers: `stddef.h`, `stdint.h`, `limits.h`, `float.h`, `stdarg.h`, `stdbool.h`, `stdalign.h`, `stdnoreturn.h`, `iso646.h` (D8).
- [ ] `<errno.h>` with `thread_local int errno` (an object, not a macro over a function).
- [ ] `test/unit/smoke`: prove that `errno` is thread-local per TLS block, that the exit status propagates, and that the TLS `.tdata` image is copied.
- **DoD extra:** `example/hello-world` links against our libc only (`-nostdlib`) and runs, using a temporary `fputs` → `__rtos_write` path.

## P1.02 — libc-test harness and formatted-output core
- [x] Fetch the `third-party/libc-test` submodule at its pinned commit (Q2 resolved; run `git submodule update --init`). Rough survey: `src/functional` 85 files, `src/regression` 72, `src/math` 206 (tests and data tables), `src/api` 80 (POSIX-heavy, so curate the ISO subset), `src/common` 14. About 55 of the 146 functional+regression tests include POSIX headers or use pthread/fork.
- [ ] Survey libc-test (`src/functional`, `src/regression`, `src/math`, `src/api`, `src/common`) and write `test/libc-test/manifest.txt` with a `run`/`skip:`/`pending:` status for every test.
- [ ] `test/libc-test/shim/`: ISO/port replacements for libc-test's `src/common` pieces (`t_printf`, `t_error`, `t_status`, and the runtest equivalent without fork).
- [ ] Build glue: one ELF per test through the tools build system (`test/libc-test/libc-test.mk` + `subdir.mk`), compiled with our headers only.
- [ ] Runner `test/libc-test/run`: per-test timeout, qemu wrapper hook for cross arches, and a `results.txt` writer.
- [ ] `make check`: build, run, and diff against `test/libc-test/expected/<arch>.txt` (fail on regressions; print improvements).
- [ ] Research `printf` engine designs (musl, picolibc tinystdio, newlib nano, mpaland/eyalroz printf, Linux `vsprintf`). Implement the format parser plus the integer, char, string, pointer and `%n` conversions; `snprintf`/`vsnprintf`/`sprintf`/`vsprintf`; and the C23 `%b`, `%wN` and `%wfN` length modifiers. Floating point is deferred to P1.08.
- **DoD extra:** the baseline is committed. Running the harness twice gives identical results.

## P1.03 — `<string.h>`, `<ctype.h>`, `<assert.h>`, `<stdbit.h>`, `<stdckdint.h>`
- [ ] Research generic word-at-a-time string algorithms (with aliasing-safe access in C23) and the known edge cases.
- [ ] All `<string.h>` functions except `strdup`/`strndup` (which wait for P1.04): `memcpy`, `memmove`, `memset`, `memset_explicit`, `memccpy`, `memchr`, `memcmp`, `strcpy`/`strncpy`/`strcat`/`strncat`, `strcmp`/`strncmp`/`strcoll`/`strxfrm`, `strchr`/`strrchr`/`strspn`/`strcspn`/`strpbrk`/`strstr`, `strtok` (`thread_local` state), `strlen`, `strerror` (message table), and `memalignment` (which is in `<stdlib.h>`).
- [ ] `<ctype.h>` for the "C" locale, table-free or table-based (decide in the research note).
- [ ] `<assert.h>`: `assert` as a variadic macro (C23) and `__assert_fail` → `fprintf(stderr)` → `abort`.
- [ ] `<stdbit.h>`: every function plus the type-generic macros, built on `__builtin_clzg`/`ctzg`/`popcountg` (with fallbacks where the builtins are missing).
- [ ] `<stdckdint.h>`: `ckd_add`/`ckd_sub`/`ckd_mul` via `__builtin_*_overflow`.
- [ ] Unit tests for `memset_explicit`, `memccpy`, `<stdbit.h>` and `<stdckdint.h>`.

## P1.04 — Memory allocator (`<stdlib.h>` part 1)
- [ ] Research allocators for embedded systems (TLSF, dlmalloc, musl mallocng/oldmalloc, picolibc nano-malloc, umm_malloc, FreeRTOS heap_4/heap_5). Choose for determinism and fragmentation, write a size estimate, and record the decision in PLAN.md.
- [ ] Finalise the heap seam (`__rtos_heap_region` vs `__rtos_sbrk`) and the lock seam (`__rtos_lock_*`, with a no-op implementation in `port/none` and a futex-free single-thread version in `port/linux` until P1.12).
- [ ] `malloc`, `calloc` (with overflow check), `realloc` (C23 semantics for size 0), `free`, `aligned_alloc`, `free_sized`, `free_aligned_sized`.
- [ ] `strdup` and `strndup`.
- [ ] Unit and stress tests: alignment, exhaustion returning `NULL` with `errno == ENOMEM`, and a randomized alloc/free fuzz test with a heap consistency checker.

## P1.05 — `<stdlib.h>` part 2, `<inttypes.h>`, `<setjmp.h>`, `<signal.h>`
- [ ] Research `strtol`-family edge cases (bases, overflow, the C23 `0b` prefix), `qsort` (research introsort vs smoothsort vs heapsort; stack use matters), and PRNG choices for `rand`.
- [ ] `atoi`/`atol`/`atoll`, `strtol`/`strtoll`/`strtoul`/`strtoull`, `abs`/`labs`/`llabs`, `div`/`ldiv`/`lldiv`, `qsort`, `bsearch` (C23 qualifier-preserving macro), `rand`/`srand` (`thread_local` state).
- [ ] `exit`, `atexit` (≥ 32 entries, no malloc required), `at_quick_exit`, `quick_exit`, `_Exit`, `abort`, `getenv` (via the seam), `system`, and `mblen`/`mbtowc`/`wctomb`/`mbstowcs`/`wcstombs` (C locale).
- [ ] `<inttypes.h>`: the format macros, `strtoimax`/`strtoumax`, `imaxabs`, `imaxdiv`.
- [ ] `<setjmp.h>`: `setjmp`/`longjmp` in `arch/x86_64/src/setjmp/*.S`, with the `jmp_buf` layout in `arch/x86_64/include/__arch_setjmp.h`.
- [ ] `<signal.h>`: a software `signal`/`raise` (a handler table with `SIG_DFL`/`SIG_IGN` semantics). No OS signals.

## P1.06 — ARM cross bring-up (armv6-m, armv7e-m, armv8-m.main)
- [ ] Toolchain files `tools/makefiles/armv6-m.mk`, `armv7e-m.mk`, `armv8-m.main.mk` (`-mthumb -mcpu=…`, float ABI per the target matrix).
- [ ] Linux port for ARM: Thumb `_start`, `svc 0` syscalls, TLS via `set_tls` and a port-provided `__aeabi_read_tp`.
- [ ] `arch/<arm>/src/setjmp/*.S` for each ARM arch (Thumb-1 for v6-m; the VFP registers for the hard-float variants). One shared `arch/arm/include/` with `jmp_buf` sized for the largest variant (D15).
- [ ] Runner: `qemu-arm -cpu max` wrapper. Verify that v6-m and v7e-m code runs correctly under qemu-user, and document any instruction caveats.
- [ ] Baselines `expected/armv6-m.txt`, `armv7e-m.txt` and `armv8-m.main.txt` match x86_64, apart from documented differences.
- **DoD extra:** from here on, every step gates on all four arches.

## P1.07 — Floating-point parsing: `strtod`, `strtof`, `strtold`, `atof`
- [ ] Research correctly rounded decimal-to-binary conversion (Clinger, Gay's `strtod`, Eisel–Lemire with a big-integer fallback, musl's `floatscan`, the fast_float paper). Hexadecimal floats, `INF`/`NAN(n-char-seq)`, rounding modes (`FLT_ROUNDS`), and `errno` on overflow/underflow.
- [ ] Implement a shared core with thin `strtof`/`strtod`/`strtold` wrappers (with D7, `strtold` == `strtod`).
- [ ] Exhaustive and random round-trip tests against known-correct vectors (from libc-test plus our own).

## P1.08 — Floating-point formatting: `%e %f %g %a`, `strfromd`/`strfromf`/`strfroml`
- [ ] Research shortest and fixed-precision binary-to-decimal conversion (Ryu and Ryu-printf, Grisu, Dragonbox, Steele–White/Dragon4, musl's big-integer approach, picolibc's dtoa). Think about table size versus speed now, because Phase 2 cares about it.
- [ ] `%e`/`%E`, `%f`/`%F`, `%g`/`%G`, `%a`/`%A`; precision and flags; `inf`/`nan`; rounding modes.
- [ ] `strfromd`, `strfromf`, `strfroml`.
- [ ] Wire the float conversion in through a single internal symbol, so P2's no-float variant (D13) can replace it.

## P1.09 — `<stdio.h>` streams
- [ ] Research `FILE` designs (musl, picolibc tinystdio, newlib, BSD stdio) with RAM per stream as the main concern.
- [ ] `FILE`, the `stdin`/`stdout`/`stderr` objects, buffering modes, `setvbuf`/`setbuf`, `fflush`, `fopen`/`freopen`/`fclose` (mode parsing, including `x` exclusive), `fread`/`fwrite`, `fgetc`/`getc`/`getchar`, `ungetc`, `fgets`, `fputc`/`putc`/`putchar`, `fputs`/`puts`, `fseek`/`ftell`/`rewind`/`fgetpos`/`fsetpos`, `feof`/`ferror`/`clearerr`, `remove`, `rename`, `tmpfile`, `tmpnam` (`thread_local` buffer), and `perror`.
- [ ] `printf`/`fprintf`/`vprintf`/`vfprintf` on top of the P1.02/P1.08 engine.
- [ ] Per-`FILE` locking through the seam (no-op until P1.12).
- [ ] Finish the I/O part of the seam in `port/linux` (`open`/`close`/`read`/`write`/`seek`/`remove`/`rename`). In `port/none`, only the standard streams go to a byte sink.

## P1.10 — `scanf` family
- [ ] Research `scanf` implementations and their edge cases (the `%[` scanset, `%n`, width, input failure vs matching failure, one-character pushback limits).
- [ ] `scanf`, `fscanf`, `sscanf`, `vscanf`, `vfscanf`, `vsscanf`, including the C23 `%b` and `%wN`/`%wfN`; floating point through P1.07's core (behind a replaceable symbol, D13).

## P1.11 — `<time.h>`, `<locale.h>`, `strerror` completion
- [ ] Research civil-from-days algorithms (e.g. Howard Hinnant's paper; clean-room) and POSIX `TZ` string parsing (Q6).
- [ ] `time`, `clock`, `difftime`, `mktime` (normalization), `timegm`, `timespec_get`, `timespec_getres`, `gmtime`/`gmtime_r`, `localtime`/`localtime_r`, `asctime`, `ctime` (the static results are `thread_local`), and `strftime` (all conversions in the C locale).
- [ ] `<locale.h>`: `setlocale` (`"C"` and `""` → `"C"`), `localeconv`.
- [ ] Time seam (`__rtos_clock_gettime`, `__rtos_clock_getres`, `__rtos_cpu_time`).

## P1.12 — `<threads.h>`, `<stdatomic.h>`, real locks, `port/none`
- [ ] Research how musl, Bionic, picolibc/newlib retarget locking and the RTOS libc layers (Zephyr, FreeRTOS+newlib) handle threads, `tss` destructors, `call_once` and static TLS allocation.
- [ ] Finalise the thread part of the seam. Implement it in `port/linux` (`clone` with `CLONE_SETTLS`, futexes, and the TLS block allocated per thread).
- [ ] `thrd_*`, `mtx_*` (plain, timed, recursive), `cnd_*`, `tss_*` (destructors with `TSS_DTOR_ITERATIONS`), `call_once`, `ONCE_FLAG_INIT`.
- [ ] `<stdatomic.h>`: all macros and types plus `atomic_flag`, using the compiler builtins. Lock-free on x86_64. Document the v6-m out-of-line atomics policy (libatomic vs a port-provided interrupt-masking implementation).
- [ ] Make the libc-internal locks real (malloc, stdio, atexit, env).
- [ ] `port/none`: a single-thread port with no-op locks and one static TLS block; `thrd_create` returns `thrd_error`.
- [ ] Unit tests: thread-local `errno`/`strtok`/`rand` per thread, concurrent malloc and stdio stress, `tss` destructor order.

## P1.13 — `<fenv.h>` and libm part 1 (exact operations)
- [ ] Research the CORE-MATH project, FreeBSD msun, musl, ARM optimized-routines and the CRlibm papers for *techniques* (clean-room). Decide the accuracy target in PLAN.md (correctly rounded vs ≤ 1 ULP, per function).
- [ ] `<fenv.h>`: x86_64 (SSE MXCSR); armv7e-m and v8-m.main (VFP FPSCR); and armv6-m soft-float (document what libgcc's soft-float exposes; `FE_*` may be limited, with the macros defined only for supported modes per the standard).
- [ ] `<math.h>` macros and classification: `fpclassify`, `isfinite`, `isinf`, `isnan`, `isnormal`, `signbit`, `iszero`, `issubnormal`, `issignaling`, `iscanonical`, and the comparison macros.
- [ ] Exact operations for f/d/l: `fabs`, `copysign`, `nan`, `nextafter`/`nexttoward`/`nextup`/`nextdown`, `fmin`/`fmax`/`fmaximum*`/`fminimum*`, `fdim`, `fma`, `ceil`/`floor`/`trunc`/`round`/`lround`/`llround`/`roundeven`/`rint`/`lrint`/`llrint`/`nearbyint`, `fromfp`/`ufromfp`/`fromfpx`/`ufromfpx`, `frexp`, `ldexp`, `scalbn`/`scalbln`, `logb`, `ilogb`/`llogb`, `modf`, `fmod`, `remainder`, `remquo`, `canonicalize`, `totalorder`/`totalordermag`, `getpayload`/`setpayload`/`setpayloadsig`, and the narrowing `fadd`/`fsub`/`fmul`/`fdiv`/`ffma`/`fsqrt` (and the `d` versions).
- [ ] Build `libm.a` (or merge it into `libc.a` with a `libm.a` stub; decide and record).

## P1.14 — libm part 2: exponentials, logarithms, powers, roots
- [ ] `sqrt`, `cbrt`, `hypot`, `rootn`, `exp`/`exp2`/`exp10`/`expm1`, `exp2m1`/`exp10m1`, `log`/`log2`/`log10`/`log1p`/`logp1`/`log2p1`/`log10p1`, `pow`, `pown`, `powr`, `compoundn` (f/d/l).
- [ ] Research notes per function family (argument reduction, polynomial generation method, table sizes).

## P1.15 — libm part 3: trigonometric and hyperbolic
- [ ] `sin`/`cos`/`tan` (with Payne–Hanek reduction for large arguments), `asin`/`acos`/`atan`/`atan2`, `sinpi`/`cospi`/`tanpi`/`asinpi`/`acospi`/`atanpi`/`atan2pi`, `sinh`/`cosh`/`tanh`/`asinh`/`acosh`/`atanh` (f/d/l).

## P1.16 — libm part 4: special functions, `<complex.h>`, `<tgmath.h>`
- [ ] `erf`, `erfc`, `tgamma`, `lgamma` (the `signgam`-free C behaviour).
- [ ] `<complex.h>`: every function, `CMPLX` macros, and Annex G special cases where `__STDC_IEC_60559_COMPLEX__` is claimed (decide whether to claim it).
- [ ] `<tgmath.h>` via `_Generic`.

## P1.17 — Conformance closure
- [ ] Header audit: generate checks from Annex B (every function prototype, macro, type and `__STDC_VERSION_*_H__`) and fix every discrepancy.
- [ ] Freestanding conformance: the freestanding headers and `<string.h>`'s freestanding subset work with `-ffreestanding` alone.
- [ ] `manifest.txt` has no `pending:` entries. Every `skip:` has a justification that refers to scope (PLAN §1).
- [ ] Every implementation-defined behaviour is documented in `docs/implementation-defined.md` (the C23 J.3 checklist for the library).
- [ ] Record the Phase 1 baselines: libc-test results and sizes per arch (the input to Phase 2).
- [ ] Tag the commit `phase-1-complete`.
