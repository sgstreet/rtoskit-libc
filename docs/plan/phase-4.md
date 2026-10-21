# Phase 4 — Comparison with Arm Optimized Routines

Prerequisite: `phase-3-complete`. Targets: `armv6-m`, `armv7e-m`, `armv7e-m-dp`,
`armv8-m.main` and `x86_64` (where AOR has a generic or x86 version).

**Goal:** measure rtoskit-libc's size and speed against
[Arm Optimized Routines](https://github.com/ARM-software/optimized-routines)
(AOR), the reference implementations Arm maintains for string functions, libm
and soft-float. The outcome is a set of comparison tables and a list of gaps,
not adopted code (PLAN D91).

**Clean-room (D91):** AOR is licensed MIT OR Apache-2.0 WITH LLVM-exception,
but the clean-room rule applies whatever the licence. AOR is fetched, built and
linked **only** into measurement programs outside `libc.a`. No AOR source is
copied, translated or read as a model for our code. A gap found here becomes a
Q* or a later step that is researched from papers and the standard in the usual
way.

**Pinned revision:** `f67e7daa5457646fad9f75580263dba654c988aa` (2026-10-16).
It is fetched with a shallow `git fetch` of that commit; the commit ID is the content check (GitHub archive downloads are blocked here, D92).

**Every step's definition of done (DoD)** is the Phase 1 DoD plus:

- The library is unchanged unless a step says otherwise. libc-test passes in
  full on every board and build type, the size flavour stays within its
  budget, and the speed flavour within its +10 % budget (+20 % for
  `math-float` and `math-mix`, D87).
- Comparison results are reproducible from a clean checkout with one command,
  and are recorded in `docs/perf/` (speed) and `docs/size/` (size) with the
  AOR revision and the toolchain version.

---

## P4.01 — Research: what AOR provides  `[x]`
- [x] Survey AOR's `string/`, `math/` and `fp/` subprojects: which routines exist per core (`string/arm` for Thumb-2 and the Armv6-M `strcmp`; `fp/armv6-m` and `fp/at32` soft-float; the C `math/` everywhere), their build (`config.mk`: `SUBS`, `ARCH`, `CROSS_COMPILE`, `CFLAGS`), their stated accuracy (ulp bounds, rounding modes, `errno` handling) and their size/speed trade-offs.
  *Every file was trial-compiled with each board's flags.*
  - *M-profile string: `strcmp` on Armv6-M only, and `memchr`/`memset`/`strlen` on the Thumb-2 cores. `memcpy` and `strcpy` are A32-only, and the Thumb-2 `strcmp` needs the A32 ISA.*
  - *fp: `fp/armv6-m` has fadd/fsub/fmul and the comparisons. `fp/at32` has the full set and assembles for Thumb-2 with `-Wa,-mimplicit-it=always`.*
  - *math: all 31 files build against our headers with `-std=c23`.*
  - *Stated accuracy: AOR's float functions are 0.51–1.1 ulp and compute in double (soft on four of our five Arm boards); its doubles are 0.51–1.01 ulp.*
- [x] Decide how AOR routines are substituted: linked ahead of `libc.a` in the bench and size programs, symbol renaming where AOR's names differ (`__strcmp_arm`, `arm_math_*`), and which of our flavours each comparison uses.
  *D92:*
  - *Archives per board (`libaor-{string,math,fp}.a`), with symbols renamed to the standard and RTABI names.*
  - *Linked before `libc.a` and libgcc with `--allow-multiple-definition`, because libgcc bundles several helpers per member; the map file is checked.*
  - *The speed flavour's objects for speed and the release build for size. math is built at `-O3` for speed and `-Os` for size.*
  - *A new `softfloat` bench program.*
- [x] Research note `docs/research/optimized-routines.md`: URLs, the routine-by-routine map to our functions, the build design, and the caveats (AOR's math assumes an FPU or a soft-float runtime; `errno` and `math_errhandling` differences).

## P4.02 — Fetch and build AOR  `[x]`
- [x] A pinned fetch (shallow `git fetch` of the revision, checked against the commit ID) into `local/`, by a make target in the style of `make setup-bench`; no submodule, nothing in `libc/`.
  *`make setup-aor` (`tools/makefiles/setup.mk`). `make aor` fetches AOR, then builds it and runs the smoke test for every board.*
- [x] Build AOR's string, math and soft-float libraries per Arm board with our toolchain and each board's `-march`/`-mfloat-abi` flags, and for `x86_64` where meaningful, into `build/<board>/<build-type>/aor/`. Record which routines build for which core.
  *D93. `tools/scripts/aor-build` writes `build/<board>/aor/` (independent of the build type).*
  - *Archives: string has 1 symbol on Armv6-M and 3 on Thumb-2. math has 41 symbols at `-O3` and again at `-Os`. fp has 32 symbols on Armv6-M and 56 on Thumb-2; x86_64 has math only.*
  - *`aor.json` lists what each archive provides, and the research note has the table.*
  - *`memchr.S`/`memset.S` are assembled without their forced `.arch armv7-a`, so they are built and checked as M-profile code.*
- [x] A smoke test: each AOR library links against our `libc.a` and runs under `TARGET_RUNNER`.
  *`tools/aor/smoke.c` passes on all six boards.*
  - *The map check found that on Armv6-M libgcc had kept its own double comparisons. `link()` now forces each AOR symbol the program contains (`-u`) and relinks until the set is stable.*
  - *The check also found a naming bug (`__leff2`), now fixed.*
  - *The library is unchanged: `make check` on linux-x86_64 release still passes 219/219 and the unit tests.*

## P4.03 — Performance comparison  `[ ]`
- [ ] `bench-runner compare` gains an AOR column: the test/bench programs relinked with AOR's routines ahead of `libc.a`, in the size and speed flavours.
- [ ] Per-function tables for string (`memcpy`, `memset`, `memchr`, `strcmp`, `strlen`, `strcpy`), libm (float and double, throughput and latency) and soft-float (`__aeabi_fadd`/`fmul`/`fdiv`, `dcmp`, conversions on Armv6-M), with the geometric means.
- [ ] Results in `docs/perf/phase-4.md`.

## P4.04 — Size comparison  `[ ]`
- [ ] Per-routine code size (`.text` + `.rodata`, tables included) for each AOR routine against ours, both flavours, per core.
- [ ] The `test/size` reference programs relinked with AOR's routines: image sizes and stack depths through `size-report`.
- [ ] Results in `docs/size/phase-4.md`.

## P4.05 — Accuracy comparison  `[ ]`
- [ ] Float functions exhaustively on the host (all 2^32 inputs, or a documented subset where too slow), double functions on random and hard-case sets: max ulp error and the count of results not correctly rounded, for AOR and for us, against an MPFR or mpmath reference.
- [ ] `errno`, special values and rounding-mode behaviour per C23 Annex F for both.
- [ ] Results in `docs/research/optimized-routines.md` (accuracy section).

## P4.06 — Analysis  `[ ]`
- [ ] For each function where AOR is faster, smaller or more accurate: the cause as far as measurements show it (algorithm, tables, register use), and whether a clean-room change could close it within our budgets.
- [ ] Gaps worth closing are recorded as Q* in PLAN.md or as new steps in this phase file; nothing is adopted here.

## P4.07 — Wrap-up  `[ ]`
- [ ] `docs/perf/phase-4.md` and `docs/size/phase-4.md` complete, with the summary tables and the commands to reproduce them.
- [ ] Recheck both budgets. Full libc-test on all boards and flavours.
- [ ] Tag `phase-4-complete` (pushed by the user: this environment cannot push tags).
