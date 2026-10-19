# Phase 2 — Size optimization pass

Prerequisite: `phase-1-complete`. Targets: `armv6-m`, `armv7e-m`, `armv7e-m-dp`,
`armv8-m.main` and `x86_64` (the last one for reference only).

**Every step's definition of done (DoD)** is the Phase 1 DoD (libc-test with no
regressions on all five arches, research notes, no new `#if`, committed) plus:

- `tools/size-report` shows that no object grew by more than 0 bytes unless the
  growth is justified in the step notes. The reductions are recorded in
  `docs/size/<arch>.md`.
- Correctness is never traded for size: the libc-test baseline must stay identical.

Metrics, per arch, in the `release` build:
(a) `.text+.rodata` per archive member;
(b) `.data+.bss` and **`.tdata+.tbss` (the per-thread cost)**;
(c) the image size of reference programs `test/size/{hello-puts, hello-printf-int,
hello-printf-float, malloc-heavy, math-mix}` linked with `--gc-sections`;
(d) the maximum stack depth of the key functions (`-fstack-usage`, `.su` files).

---

## P2.01 — Measurement infrastructure  `[x]`
- [x] `tools/size-report` (a script): per-member `size`, `.su` stack data, and the reference program images, written as a committed JSON or Markdown baseline per arch (`docs/size/baseline-<arch>.json`). *`tools/scripts/size-report report` (D51): the section classes per member from the ELF flags, the images split by origin from the link map, the frames and the worst stack depths from `-fcallgraph-info=su`, now on every compile. `markdown` prints the summary in `docs/size/<arch>.md`.*
- [x] A diff tool that compares the current build against the baseline and highlights the biggest regressions. *`tools/scripts/size-report diff`: the totals, the largest growths and reductions per member, the images, the stack depths and the frames. Exit status 1 when anything grew. Checked by fault injection (a padded `abs`).*
- [x] Add `test/size/*` reference programs to the build. *`hello-puts`, `hello-printf-int`, `hello-printf-float`, `malloc-heavy`, `math-mix`; built for every board and build type, measured rather than run (run by hand on x86_64, armv6-m and armv7e-m-dp). The x86_64 board now compiles with `-muclibc` so GCC does not fuse `sin`/`cos` into `sincos`.*
- [x] Capture the Phase 1 baseline. Do a comparison run against picolibc and newlib-nano (the same reference programs) for context only. *`docs/size/baseline-<arch>.json` and `docs/size/<arch>.md` for the six boards; `docs/size/comparison.md` (`size-report compare`: picolibc 1.8.6 and newlib-nano 4.4 from pinned Ubuntu packages; the toolchain's own picolibc 1.8.12 is slim LTO and does not link). The library itself is unchanged, so libc-test stays 219/219 on every board in both build types.*

## P2.02 — Global build and structure levers
- [ ] Evaluate the flags: `-Os` vs `-Oz` (clang), `-fno-unwind-tables -fno-asynchronous-unwind-tables`, `-fno-common`, `-fshort-enums` (ABI check!), `-flto` for archives (on/off, measured), `-mslow-flash-data`, and `-fno-plt` (irrelevant in a static link). Adopt per arch in `tools/makefiles/<arch>.mk`.
- [ ] Audit for accidental pull-in: a link-map dependency graph from the reference programs. For example, `assert` → `fprintf` → the full stdio engine; `abort` → `raise` → the signal table. Break the chains with smaller internal entry points.
- [ ] Audit `thread_local` usage: the TLS bytes per thread for each reference program; shrink or share buffers where the C standard allows.

## P2.03 — stdio and printf flavours (D13)
- [ ] Research the size-tiered printf designs (newlib-nano's weak `_printf_float`, picolibc's `PICOLIBC_*_PRINTF_SCANF` link-time aliasing, Zephyr cbprintf).
- [ ] A link-time selectable `libc-nofloat.a` (the float conversions are stubbed) and an optional `libc-minimal-printf.a` (no `%n`, no wide length modifiers). Each is a separate archive built from file selection, never `#if`.
- [ ] Reduce `FILE` size and the per-stream buffers; std streams with no static buffers when unbuffered.
- [ ] Do the same for `scanf`.

## P2.04 — `<stdlib.h>`, `malloc`, `<string.h>`, `<time.h>`
- [ ] Shrink the allocator's code and per-block overhead; an optional "tiny" allocator in a separate archive.
- [ ] `qsort` code size; `strerror` table compaction; `ctype` (table vs code) re-evaluation per arch.
- [ ] `strftime`/`mktime`/TZ parsing code size.

## P2.05 — libm size
- [ ] Share the reduction and polynomial kernels between the `f`/`d` variants where accuracy allows. Re-derive the table sizes with the size/accuracy trade-off documented.
- [ ] armv7e-m and v8-m.main: the `float` functions must use only the single-precision FPU (no double promotion; enforce with `-Wdouble-promotion`).
- [ ] armv7e-m-dp: the `double` functions must not pull in any soft-float helpers (`__aeabi_d*`); check the link maps. Use the hardware ops instead of software bit tricks wherever that is smaller.
- [ ] armv6-m: minimise the soft-float helper pull-in (audit the libgcc dependencies).

## P2.06 — Per-arch hand-written size kernels
- [ ] armv6-m Thumb-1: `memcpy`, `memset`, `memmove`, `strlen`, `strcmp`, `setjmp`, written for size (the arch `size/` flavour, D12).
- [ ] armv7e-m, armv7e-m-dp and v8-m.main Thumb-2: the same set (one shared Thumb-2 subdirectory), if they beat the compiler output.
- [ ] Each assembly file has a research note and goes through libc-test plus our own exhaustive alignment/length tests.

## P2.07 — Phase wrap-up
- [ ] Final size report against the Phase 1 baseline and against the picolibc/newlib-nano comparison.
- [ ] Document the flavour/archive selection for users (`docs/configuration.md`).
- [ ] Lock the size budget per reference program per arch (Phase 3 must stay within budget +N% for the speed flavour; N is decided here).
- [ ] Tag `phase-2-complete`.
