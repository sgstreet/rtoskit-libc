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

## P2.02 — Global build and structure levers  `[x]`
- [x] Evaluate the flags: `-Os` vs `-Oz` (clang), `-fno-unwind-tables -fno-asynchronous-unwind-tables`, `-fno-common`, `-fshort-enums` (ABI check!), `-flto` for archives (on/off, measured), `-mslow-flash-data`, and `-fno-plt` (irrelevant in a static link). Adopt per arch in `tools/makefiles/<arch>.mk`. *D52. Every flag, plus `-fno-jump-tables`, `-fno-inline-small-functions` and `-fipa-pta`, was measured on all boards. Adopted: no unwind tables on x86_64 (`libc.a` −28 %). Rejected: `-Oz` (identical on Arm), `-fshort-enums` (ABI), `-mslow-flash-data` (no TLS support, +3.9 %), LTO (2–18 % per image for library-only LTO, but tied to one GCC: Q13).*
- [x] Audit for accidental pull-in: a link-map dependency graph from the reference programs. For example, `assert` → `fprintf` → the full stdio engine; `abort` → `raise` → the signal table. Break the chains with smaller internal entry points. *Six chains are broken with weak references and colocated tables: `exit` → the atexit table and lock, `abort` → `raise`, `assert` → `snprintf`, `asctime` → `snprintf`, `exit` → `fclose`, and the flush of both standard streams. On armv6-m an empty `main` goes from 1388 to 506 bytes, `assert` from 9665 to 3245 and `asctime` from 7788 to 1572. Every reference image shrank. Tests: `asctime` single-digit fields and a `stdio` `exit-file` mode (`--wrap=__rtos_close`); six faults injected and caught.*
- [x] Audit `thread_local` usage: the TLS bytes per thread for each reference program; shrink or share buffers where the C standard allows. *The reference programs carry 4 bytes (`errno`), and `math-mix` now 0. The `asctime` result shrinks from 64 to 26 bytes, the length C23 defines. The tss values (256), `struct tm` (36, already shared), `tmpnam` (32), `rand` (8) and `strtok` (4) are at their minimum, and their results may be live at the same time, so they cannot share.*

## P2.03 — stdio and printf flavours (D13)  `[x]`
- [x] Research the size-tiered printf designs (newlib-nano's weak `_printf_float`, picolibc's `PICOLIBC_*_PRINTF_SCANF` link-time aliasing, Zephyr cbprintf). *`docs/research/stdio-flavours.md`.*
- [x] A link-time selectable `libc-nofloat.a` (the float conversions are stubbed) and an optional `libc-minimal-printf.a` (no `%n`, no wide length modifiers). Each is a separate archive built from file selection, never `#if`. *D53. Both are complete archives built in `libc/libc.mk` from `libc/stdio/{nofloat,minimal}/`. Floating conversions fail with `EINVAL`. The minimal flavour also refuses `%n` and the lengths wider than `long`. armv6-m `hello-printf-int`: 8249 → 5351 (nofloat) → 4779 (minimal); armv7e-m minimal 4271, with no libgcc. Tested by `test/unit/flavours`, linked three times, and two new `test/size` programs.*
- [x] Reduce `FILE` size and the per-stream buffers; std streams with no static buffers when unbuffered. *The FILE drops from 36 to 32 bytes (singly linked list). `stdin`/`stdout` have static buffers linked only with the stream, and `stderr` has none. Setup names `malloc` weakly, so `hello-puts` links no allocator: −368 bytes of code; the 256 bytes of RAM move from the heap to `.bss`. `setvbuf(NULL)` keeps the stream's buffer.*
- [x] Do the same for `scanf`. *`__libc_scan_float` is stubbed in `libc-nofloat.a` (a matching failure, `EINVAL`, no `strtod` engine). `__libc_scan_store`/`__libc_scan_count` are replaceable, so `libc-minimal-printf.a` refuses `%n` and the stores wider than `long` in the scanf family too. A `sscanf` probe drops from 13332 to 3194 bytes on armv6-m with `libc-nofloat.a`.*

## P2.04 — `<stdlib.h>`, `malloc`, `<string.h>`, `<time.h>`  `[x]`
- [x] Shrink the allocator's code and per-block overhead; an optional "tiny" allocator in a separate archive. *D54. nano is already at one header word and the alignment `max_align_t` needs, so it is unchanged. `libc/malloc/tiny` uses an implicit list with deferred merging, a sentinel and the first region only. It is 13 % smaller than nano (574 vs 658 bytes on armv6-m), ships as `libc-tinymalloc.a` (or `LIBC_MALLOC=tiny`), and passes the full malloc test as `malloc-tiny`. An allocate-only design was rejected (7.24.3.3).*
- [x] `qsort` code size; `strerror` table compaction; `ctype` (table vs code) re-evaluation per arch. *`strerror` is one array of records: 2395 → 1968 bytes, with a golden test of every number. ctype stays as code (14–34 bytes per function against a 256-byte table), and so does `qsort` (a 228-byte heapsort). Library-wide, 64-bit division by small constants now goes through `__libc_udivmod64`, so printf images on armv7e-m/v8-m drop all libgcc (−648 bytes).*
- [x] `strftime`/`mktime`/TZ parsing code size. *The calendar, `strftime`, `asctime` and the TZ rules divide through `__libc_udivmod64`/`__libc_floor_divmod64` instead of libgcc's 64-bit division. A localtime + strftime + mktime probe: 6344 → 5470 bytes on armv7e-m (no libgcc) and 6904 → 6122 on armv6-m. The code itself was already table-light (`docs/research/stdlib-time-size.md`).*

## P2.05 — libm size (split into P2.05a–e: about 90 `float` functions compute in `double`, which is soft-float on the single-precision FPUs)

## P2.05a — Soft-float helper audit: armv7e-m-dp and armv6-m  `[x]`
- [x] armv7e-m-dp: the `double` functions must not pull in any soft-float helpers (`__aeabi_d*`); check the link maps. Use the hardware ops instead of software bit tricks wherever that is smaller.
  *64-bit conversions through two exact 32-bit VCVTs (fpv5-dp layer: `__libc_u64_to_double`, `__libc_i64_to_double`, `__libc_double_to_i64`), `strtof` through 32 bits; no `libc.a` member refers to a double helper; `math-mix` 13244 → 12326 with no libgcc (D55). VCVT for `lrint`/`lround` rejected: it saturates instead of the uniform `LONG_MIN`.*
- [x] armv6-m: minimise the soft-float helper pull-in (audit the libgcc dependencies).
  *`nofp` layer: `__aeabi_dsub`/`__aeabi_fsub` as addition of the negation (−1444/−764 bytes of libgcc); special-case results and exception raising out of line in `libc/fenv`, constants on the soft variant (no soft division); `math-mix` 30024 → 27864, libgcc share 9368 → 7412. Comparisons and division left to libgcc (fragile to replace; P2.06). Test `test/unit/softfp`, 8 mutants caught (D55).*

## P2.05b — Single-precision layer: infrastructure and the exact `float` functions  `[x]`
- [x] A layer for armv7e-m and v8-m.main whose `float` functions use only the single-precision FPU (no double promotion; enforce with `-Wdouble-promotion` and a check that its objects reference no `__aeabi_d*` helper).
  *`libc/math/sp`, first in `LIBC_MATH_LAYERS` on every Arm core (armv6-m and armv7e-m-dp measured smaller too); `subdir.mk` yields to later FPU layers; `sp.checked` fails the build on any double helper or double-computing 64-bit conversion (D56).*
- [x] The exact and bit-level `float` functions: rounding, `fmodf`/`remainderf`/`remquof`, `frexpf`/`ldexpf`/`scalbnf`, `ilogbf`/`logbf`, `nextafterf`/`nexttowardf`, `l(l)rintf`/`l(l)roundf`, `fromfpf`, `modff`, the trivial complex accessors.
  *`ldexpf`/`scalbnf`/`nextafterf` were already float-only. New reference program `float-exact`: 5056 → 2048 (armv7e-m, no libgcc), 8440 → 3908 (armv6-m). Test `test/unit/float_exact` (bitwise against the double functions, 40 000 arguments each), 11 mutants caught.*

## P2.05c — Single-precision exp, log and pow family (split into P2.05c1–c3)  `[x]`
- `expf`, `exp2f`, `exp10f`, `expm1f` and the `m1` forms, `logf`, `log2f`, `log10f`, `log1pf` and the `p1` forms, `powf`, `pownf`, `powrf`, `rootnf`, `rsqrtf`, `cbrtf`, `hypotf`, `compoundnf`, within the D47 accuracy, on single-precision float-float cores.

## P2.05c1 — Float-float infrastructure and the exponentials  `[x]`
- [x] Float-float arithmetic (`struct __libc_ff`), the `exp2` core and its scaling, a `sp-nearest` layer for the soft-`<fenv.h>` cores (round to nearest, no flags), the shared accuracy test.
  *`test/unit/float_accuracy`: max error < 1 ulp and at most 1/1000 not correctly rounded; on x86_64 the layer runs natively (4M arguments per function plus every subnormal-result argument). 9 mutants caught (D57).*
- [x] `expf`, `exp2f`, `exp10f`, `expm1f`, `exp2m1f`, `exp10m1f`.
  *Max 0.500–0.510 ulp, a few in 10^5 not correctly rounded. Images grow until the remaining float functions follow (transitional, justified in `docs/size`).*

## P2.05c2 — The logarithms  `[x]`
- [x] The `log2` core; `logf`, `log2f`, `log10f`, `log1pf`/`logp1f`, `log2p1f`, `log10p1f`.
  *Every log 0.500 ulp, at most 2 in 10^5 not correctly rounded; the series' exact square was needed (D58). Test bound tightened to 1/10 000; 8 mutants caught.*

## P2.05c3 — Powers and roots  `[x]`
- [x] `powf`, `pownf`, `powrf`, `rootnf`, `compoundnf`, `rsqrtf`, `cbrtf`, `hypotf`.
  *Plus a float-only `sqrtf` in `sp` (armv6-m). All within 0.509 ulp; 12 mutants (11 caught, 1 numerically insignificant). Fixed the double `compoundn` for tiny x and large n, and the sign of zero in `__libc_exp2f_scale` (D59).*

## P2.05d — Single-precision trigonometric and hyperbolic functions (split into P2.05d1–d3)  `[x]`
- `sinf`, `cosf`, `tanf` (with a single-precision reduction), the pi-scaled forms, the inverse and hyperbolic functions, within the D48 accuracy.

## P2.05d1 — sin, cos, tan and the pi-scaled forms  `[x]`
- [x] An exact integer argument reduction (`__libc_rem_pio2f`) and a Tang sin core; `sinf`, `cosf`, `tanf`, `sinpif`, `cospif`, `tanpif`.
  *All 0.500 ulp; 10 mutants (8 caught, 1 equivalent, 1 marginal at 2^-12 ulp) (D60).*

## P2.05d2 — The inverse trigonometric functions  `[x]`
- [x] An atan2 core; `atanf`, `atan2f`, `asinf`, `acosf` and their pi-scaled forms.
  *All 0.500 ulp; 9 mutants (7 caught, 2 benign) (D61).*

## P2.05d3 — The hyperbolic functions  `[x]`
- [x] `sinhf`, `coshf`, `tanhf`, `asinhf`, `acoshf`, `atanhf`.
  *All 0.500 ulp; the test gained a 0.52-ulp bound (D62).*

## P2.05e — Single-precision special and complex functions; shared kernels (split into P2.05e1–e4)
- `erff`, `erfcf`, `lgammaf`, `tgammaf` and the `float complex` functions on the single-precision layer.
- Share the reduction and polynomial kernels between the `f`/`d` variants where accuracy allows. Re-derive the table sizes with the size/accuracy trade-off documented.

## P2.05e1 — The error functions  `[x]`
- [x] `erff` and `erfcf` on float-float, within the D47 accuracy.
  *Both 0.500 ulp, from the series with positive terms and Laplace's continued fraction; 14 mutants (12 caught, 2 equivalent) (D63).*

## P2.05e2 — The gamma functions
- [ ] `lgammaf` and `tgammaf` on float-float, within the D47 accuracy.

## P2.05e3 — The `float complex` functions
- [ ] The `float complex` functions on the single-precision cores (a complex `sp-nearest` layer), within the D49 accuracy.

## P2.05e4 — Kernel sharing and table sizes
- [ ] Share the reduction and polynomial kernels between the `f`/`d` variants where accuracy allows; re-derive the table sizes with the size/accuracy trade-off documented.

## P2.06 — Per-arch hand-written size kernels
- [ ] armv6-m Thumb-1: `memcpy`, `memset`, `memmove`, `strlen`, `strcmp`, `setjmp`, written for size (the arch `size/` flavour, D12).
- [ ] armv7e-m, armv7e-m-dp and v8-m.main Thumb-2: the same set (one shared Thumb-2 subdirectory), if they beat the compiler output.
- [ ] Each assembly file has a research note and goes through libc-test plus our own exhaustive alignment/length tests.

## P2.07 — Phase wrap-up
- [ ] Final size report against the Phase 1 baseline and against the picolibc/newlib-nano comparison.
- [ ] Document the flavour/archive selection for users (`docs/configuration.md`).
- [ ] Lock the size budget per reference program per arch (Phase 3 must stay within budget +N% for the speed flavour; N is decided here).
- [ ] Tag `phase-2-complete`.
