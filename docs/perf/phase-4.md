# Phase 4: rtoskit-libc against Arm Optimized Routines

Plan: `docs/plan/phase-4.md`. Decisions: PLAN D91, D92, D93. Research and
caveats: `docs/research/optimized-routines.md`.

AOR revision `f67e7daa5457646fad9f75580263dba654c988aa`, built with arm-none-eabi
GCC 16.2 (and gcc-14 on x86_64) by `tools/scripts/aor-build`. AOR is measured
only; nothing of it is in `libc.a`.

## Speed (P4.03)

### Method

`make aor`, then `tools/scripts/bench-runner aor --record`.

- **Programs:** the speed build's objects of the `test/bench` programs,
  relinked with AOR's `libaor-string.a`, `libaor-math.a` (AOR's own `-O3`)
  and `libaor-fp.a` ahead of the same `libc.a`. Every AOR routine the program
  contains replaces ours or libgcc's, and the map check proves it.
- **Measurement:** instructions per operation are counted by the same qemu
  plugin as `make bench`. They are compared with the size and speed baselines
  in `docs/perf/baseline-<arch>[-speed].json`.
- **Records:** the results are in `docs/perf/aor-<arch>.json`, and
  `bench-runner aor --from-record` prints the tables below.
- **New `softfloat` program:** floating-point operations that become library
  calls without an FPU. It joined `test/bench` in this step, and its cases are
  in every board's baselines and `docs/perf/<arch>.md`.

### Caveats that shape the numbers

- **Soft-float contracts differ.**
  - AOR's soft-float ignores the rounding mode and raises no exception flags.
  - Ours (`libc/math/nofp`, Armv6-M `fmul`/`fdiv`/`fsub`/`dsub`) honours the
    soft `<fenv.h>`.
  - libgcc's (every other helper in our builds) also ignores both.
- **Accuracy differs.**
  - AOR's float functions are 0.51–1.1 ulp and compute in double.
  - Ours are correctly rounded to nearest on the cores with a double FPU or
    SSE2, and below 1 ulp elsewhere (D45, D47, D57).
  - P4.05 measures both.
- **The ratio's scope.** "AOR / speed" is AOR's count over our speed flavour's
  count for the same case. Where AOR has no routine, a program can still change
  through AOR's soft-float helpers running inside our functions; on Armv6-M
  every libm case changes this way.
- **erf on cores without FMA.** AOR's `erf` and `erff` evaluate their
  polynomials with explicit `fma` calls. On x86_64 (the SSE2 baseline has no
  FMA) and for double on Arm, every such call is our exact software `fma`. That
  is why they are 4–9 times our count on x86_64. On Arm the float FMA is an
  instruction from FPv4 on.

### Summary

Geometric mean of AOR / speed over the cases whose count changed; below 1
means AOR is faster.

| Program | x86_64 | armv6-m | armv6s-m | armv7e-m | armv7e-m-dp | armv8-m.main |
|---|---:|---:|---:|---:|---:|---:|
| string | — | 0.96 | 0.96 | 0.95 | 0.95 | 0.95 |
| stdio | — | 0.99 | 0.99 | 0.99 | 1.00 | 0.99 |
| strto | — | 0.96 | 0.96 | 0.70 | 1.00 | 0.70 |
| malloc | — | — | — | 1.02 | 1.02 | 1.02 |
| qsort | — | 1.06 | 1.06 | — | — | — |
| math | 0.30 | 0.80 | 0.80 | 0.81 | 0.61 | 0.81 |
| softfloat | — | 0.59 | 0.59 | 0.47 | 0.79 | 0.47 |

### What the numbers say

**String.** AOR has only four M-profile string routines.

- **`memchr` (Thumb-2):** AOR's takes about half our instructions from 16
  bytes up: 1323 against 2615 at 1 KiB. Ours is the generic C in both
  flavours, so this is the largest string gap.
- **`strlen` (Thumb-2):** 21 % fewer at 1 KiB (1034 against 1315). Ours stays
  the Thumb-1 kernel because of the speed budget (Q16), and the measured gap
  matches what P3.02b expected.
- **`memset` (Thumb-2):** ours (P3.02a) is ahead from 256 bytes (177 against
  229 at 1 KiB) and level below.
- **`strcmp` (Armv6-M):** AOR's word loop takes 0.29 of ours at 1 KiB (2087
  against 7198). It costs more for strings of 0–1 bytes (39 against 30), and
  `qsort/strings/256`, whose keys differ early, is 6 % slower with it.
- **`malloc` (Thumb-2):** the one changed case is `calloc/256`, whose 256-byte `memset` costs 2 more instructions with AOR's.

**Soft-float, Armv6-M.**

- AOR's Thumb-1 `fadd`, `fsub`, `fmul` and the comparisons take 0.52–0.61 of
  ours: `fmul` 56 against 109, `fcmp` 24 against 44, `dcmp` 24 against 67.
  - Our `fmul` and `fsub` carry the soft `<fenv.h>` contract.
  - Our comparisons are libgcc's.
- AOR has no Thumb-1 division, double arithmetic or conversions, so those
  cases do not change. `f2l`/`d2l` improve 16–19 % only because libgcc's
  conversions call AOR's comparisons.

**Soft-float, single-precision FPU (armv7e-m, armv8-m.main).**

- AOR's at32 double helpers take 0.14–0.95 of libgcc's: `ddiv` 82 against 583,
  `f2l`/`d2l` 21–26 against 141–178, `i2d` 19 against 41, `dcmp` 20 against 49,
  `dadd` 46 against 64.
- These are all libgcc's in our builds; rtoskit-libc has no double soft-float
  of its own on these cores. This is why `strto` drops to 0.70 there:
  `strtod`'s exact paths divide in double.

**Soft-float, armv7e-m-dp.** Only the 64-bit integer conversions are calls:
`f2l` 21 against 26, and `l2d` 30 against 49.

**libm, double functions.**

- **The AOR functions:** `exp`, `exp2`, `log`, `log2` and `pow` take 0.16–0.39
  of ours on Armv6-M and the single-precision cores, 0.28–0.45 on armv7e-m-dp,
  and 0.18–0.28 on x86_64.
  - Our cores (D47) are double-double, aiming at below 1 ulp with a margin.
  - AOR's are table-driven and finish in double precision, at about 0.52 ulp.
  - This is the largest systematic gap the comparison shows.
- **Our other double functions** (`sin`, `atan`, `erfc`, ...) get 0.39–0.83 on
  the single-precision cores from AOR's soft-double helpers alone, which
  matches the `softfloat` figures.

**libm, float functions.**

- **Double or SSE2 FPU (armv7e-m-dp, x86_64):** AOR's double-evaluated float
  functions take 0.14–0.31 of ours. The exception is `erff` on x86_64 (the
  `fma` caveat above).
- **Single-precision FPU (armv7e-m, armv8-m.main):** AOR's float functions are
  4–4.5 times ours (`expf` 681 against 160), because every double operation in
  them is a soft-float call. `sinf`/`cosf` are 1.3 times ours. `erff` is 0.40
  of ours (550 against 1383).
- **Armv6-M:** our speed flavour's fixed-point paths (D86–D88) beat AOR's
  `expf`, `exp2f`, `log2f` and `erff` 3.6–7 times. AOR's `log10f` and `powf`,
  which have no fixed-point path here, take 0.22–0.25 of ours. `logf` is level,
  and `sinf`/`cosf` take 0.65–0.82.

The per-function tables for every board follow. P4.06 turns the gaps into Q*
entries or later steps.

## Per-board tables

`tools/scripts/bench-runner aor --from-record`.

### x86_64

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 0 of 603 | — | — |
| stdio | 0 of 23 | — | — |
| strto | 0 of 21 | — | — |
| malloc | 0 of 9 | — | — |
| qsort | 0 of 8 | — | — |
| math | 30 of 124 | 0.30 | `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `pow`, `powf`, `sinf` |
| softfloat | 0 of 20 | — | — |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `expf` | 237 | 237 | 43.7 | 0.18 |
| `exp2f` | 294 | 294 | 42.7 | 0.15 |
| `logf` | 259 | 259 | 49.7 | 0.19 |
| `log2f` | 337 | 337 | 49.7 | 0.15 |
| `log10f` | 295 | 295 | 52.7 | 0.18 |
| `powf` | 633 | 633 | 88.6 | 0.14 |
| `sinf` | 332 | 332 | 52.6 | 0.16 |
| `cosf` | 320 | 320 | 52.7 | 0.16 |
| `erff` | 511 | 511 | 2171 | 4.25 |
| `exp` | 230 | 230 | 55.7 | 0.24 |
| `exp2` | 287 | 287 | 51.7 | 0.18 |
| `log` | 252 | 252 | 69.7 | 0.28 |
| `log2` | 330 | 330 | 81.7 | 0.25 |
| `pow` | 625 | 625 | 165 | 0.26 |
| `erf` | 504 | 504 | 4400 | 8.73 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 7.6 | 7.6 | 7.6 | 1.00 |
| `fsub` | 7.6 | 7.6 | 7.6 | 1.00 |
| `fmul` | 7.6 | 7.6 | 7.6 | 1.00 |
| `fdiv` | 7.6 | 7.6 | 7.6 | 1.00 |
| `fcmp` | 8.6 | 8.6 | 8.6 | 1.00 |
| `dadd` | 10.6 | 10.6 | 10.6 | 1.00 |
| `dsub` | 10.6 | 10.6 | 10.6 | 1.00 |
| `dmul` | 10.6 | 10.6 | 10.6 | 1.00 |
| `ddiv` | 10.6 | 10.6 | 10.6 | 1.00 |
| `dcmp` | 8.6 | 8.6 | 8.6 | 1.00 |
| `f2d` | 9.6 | 9.6 | 9.6 | 1.00 |
| `d2f` | 6.6 | 6.6 | 6.6 | 1.00 |
| `f2i` | 9.6 | 9.6 | 9.6 | 1.00 |
| `i2f` | 6.6 | 6.6 | 6.6 | 1.00 |
| `d2i` | 9.6 | 9.6 | 9.6 | 1.00 |
| `i2d` | 9.6 | 9.6 | 9.6 | 1.00 |
| `f2l` | 8.6 | 8.6 | 8.6 | 1.00 |
| `l2f` | 6.6 | 6.6 | 6.6 | 1.00 |
| `d2l` | 8.6 | 8.6 | 8.6 | 1.00 |
| `l2d` | 9.6 | 9.6 | 9.6 | 1.00 |

### armv6-m

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 60 of 603 | 0.96 | `strcmp` |
| stdio | 1 of 23 | 0.99 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fmul`, `__gedf2`, `__ledf2`, `strcmp` |
| strto | 14 of 21 | 0.96 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fmul`, `__gedf2`, `__ledf2`, `strcmp` |
| malloc | 0 of 9 | — | `strcmp` |
| qsort | 1 of 8 | 1.06 | `strcmp` |
| math | 124 of 124 | 0.80 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_cfcmpeq`, `__aeabi_cfcmple`, `__aeabi_cfrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpge`, `__aeabi_fcmpgt`, `__aeabi_fcmple`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__gesf2`, `__ledf2`, `__lesf2`, `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `pow`, `powf`, `sinf`, `strcmp` |
| softfloat | 7 of 20 | 0.59 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fadd`, `__aeabi_fcmplt`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__ledf2`, `strcmp` |

#### string

Aligned case per size: size flavour / speed flavour / AOR.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `strcmp` | 29 / 30 / 39 | 37 / 37 / 45 | 61 / 58 / 47 | 157 / 142 / 71 | 541 / 478 / 167 | 2077 / 1822 / 551 | 8221 / 7198 / 2087 |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `sqrtf` | 461 | 461 | 429 | 0.93 |
| `cbrtf` | 8590 | 8590 | 5292 | 0.62 |
| `hypotf` | 6293 | 6293 | 3747 | 0.60 |
| `expf` | 11034 | 375 | 2452 | 6.54 |
| `exp2f` | 9269 | 316 | 2212 | 6.99 |
| `expm1f` | 12310 | 12310 | 6768 | 0.55 |
| `logf` | 12367 | 2210 | 2426 | 1.10 |
| `log2f` | 10547 | 583 | 2447 | 4.20 |
| `log10f` | 12365 | 12365 | 2686 | 0.22 |
| `log1pf` | 12789 | 12789 | 7090 | 0.55 |
| `powf` | 22158 | 22158 | 5462 | 0.25 |
| `sinf` | 18350 | 4729 | 3059 | 0.65 |
| `cosf` | 18281 | 3742 | 3077 | 0.82 |
| `tanf` | 36223 | 5140 | 4024 | 0.78 |
| `asinf` | 23011 | 23011 | 13659 | 0.59 |
| `acosf` | 23828 | 23828 | 14158 | 0.59 |
| `atanf` | 17750 | 17750 | 10464 | 0.59 |
| `atan2f` | 18422 | 18422 | 10899 | 0.59 |
| `sinhf` | 15310 | 15310 | 8735 | 0.57 |
| `coshf` | 15181 | 15181 | 8662 | 0.57 |
| `tanhf` | 15977 | 15977 | 9115 | 0.57 |
| `asinhf` | 19353 | 19353 | 11011 | 0.57 |
| `acoshf` | 19926 | 19926 | 11303 | 0.57 |
| `atanhf` | 15868 | 15868 | 8951 | 0.56 |
| `erff` | 141024 | 2967 | 10570 | 3.56 |
| `erfcf` | 119604 | 2874 | 2770 | 0.96 |
| `lgammaf` | 36729 | 36729 | 20889 | 0.57 |
| `tgammaf` | 55326 | 55326 | 31220 | 0.56 |
| `fmodf` | 273 | 273 | 230 | 0.84 |
| `floorf` | 105 | 105 | 70.7 | 0.67 |
| `roundf` | 103 | 103 | 68.7 | 0.66 |
| `sqrt` | 4208 | 4208 | 4075 | 0.97 |
| `cbrt` | 14545 | 14545 | 14512 | 1.00 |
| `hypot` | 14812 | 14812 | 14406 | 0.97 |
| `exp` | 10356 | 10356 | 3954 | 0.38 |
| `exp2` | 14172 | 14172 | 3335 | 0.24 |
| `expm1` | 10978 | 10978 | 10749 | 0.98 |
| `log` | 17003 | 17003 | 4335 | 0.25 |
| `log2` | 21459 | 21459 | 5211 | 0.24 |
| `log10` | 21106 | 21106 | 20900 | 0.99 |
| `log1p` | 18064 | 18064 | 17859 | 0.99 |
| `pow` | 31562 | 31562 | 12178 | 0.39 |
| `sin` | 17710 | 17710 | 17482 | 0.99 |
| `cos` | 17700 | 17700 | 17448 | 0.99 |
| `tan` | 32504 | 32490 | 32298 | 0.99 |
| `asin` | 34679 | 34665 | 34315 | 0.99 |
| `acos` | 35771 | 35757 | 35425 | 0.99 |
| `atan` | 21229 | 21215 | 20980 | 0.99 |
| `atan2` | 22656 | 22642 | 22258 | 0.98 |
| `sinh` | 17881 | 17881 | 17626 | 0.99 |
| `cosh` | 16736 | 16736 | 16481 | 0.98 |
| `tanh` | 20206 | 20192 | 19974 | 0.99 |
| `asinh` | 39824 | 39824 | 39448 | 0.99 |
| `acosh` | 34288 | 34288 | 33915 | 0.99 |
| `atanh` | 25888 | 25874 | 25673 | 0.99 |
| `erf` | 40695 | 40695 | 25491 | 0.63 |
| `erfc` | 112996 | 112996 | 112679 | 1.00 |
| `lgamma` | 47161 | 47123 | 46702 | 0.99 |
| `tgamma` | 64810 | 64762 | 64248 | 0.99 |
| `fmod` | 486 | 486 | 464 | 0.95 |
| `floor` | 212 | 212 | 204 | 0.96 |
| `round` | 210 | 210 | 202 | 0.96 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 90.3 | 90.3 | 50.6 | 0.56 |
| `fsub` | 84.8 | 84.8 | 51.6 | 0.61 |
| `fmul` | 109 | 109 | 56.3 | 0.52 |
| `fdiv` | 292 | 292 | 292 | 1.00 |
| `fcmp` | 44.4 | 44.4 | 24.2 | 0.54 |
| `dadd` | 107 | 107 | 107 | 1.00 |
| `dsub` | 116 | 116 | 116 | 1.00 |
| `dmul` | 264 | 264 | 264 | 1.00 |
| `ddiv` | 844 | 844 | 844 | 1.00 |
| `dcmp` | 66.9 | 66.9 | 24.2 | 0.36 |
| `f2d` | 32.4 | 32.4 | 32.4 | 1.00 |
| `d2f` | 51.5 | 51.5 | 51.5 | 1.00 |
| `f2i` | 24.9 | 24.9 | 24.9 | 1.00 |
| `i2f` | 76.6 | 76.6 | 76.6 | 1.00 |
| `d2i` | 38.4 | 38.4 | 38.4 | 1.00 |
| `i2d` | 61.6 | 61.6 | 61.6 | 1.00 |
| `f2l` | 689 | 689 | 580 | 0.84 |
| `l2f` | 525 | 525 | 525 | 1.00 |
| `d2l` | 696 | 696 | 568 | 0.81 |
| `l2d` | 473 | 473 | 473 | 1.00 |

### armv6s-m

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 60 of 603 | 0.96 | `strcmp` |
| stdio | 1 of 23 | 0.99 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fmul`, `__gedf2`, `__ledf2`, `strcmp` |
| strto | 14 of 21 | 0.96 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fmul`, `__gedf2`, `__ledf2`, `strcmp` |
| malloc | 0 of 9 | — | `strcmp` |
| qsort | 1 of 8 | 1.06 | `strcmp` |
| math | 124 of 124 | 0.80 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_cfcmpeq`, `__aeabi_cfcmple`, `__aeabi_cfrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpge`, `__aeabi_fcmpgt`, `__aeabi_fcmple`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__gesf2`, `__ledf2`, `__lesf2`, `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `pow`, `powf`, `sinf`, `strcmp` |
| softfloat | 7 of 20 | 0.59 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_fadd`, `__aeabi_fcmplt`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__ledf2`, `strcmp` |

#### string

Aligned case per size: size flavour / speed flavour / AOR.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `strcmp` | 29 / 30 / 39 | 37 / 37 / 45 | 61 / 58 / 47 | 157 / 142 / 71 | 541 / 478 / 167 | 2077 / 1822 / 551 | 8221 / 7198 / 2087 |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `sqrtf` | 461 | 461 | 429 | 0.93 |
| `cbrtf` | 8590 | 8590 | 5292 | 0.62 |
| `hypotf` | 6293 | 6293 | 3747 | 0.60 |
| `expf` | 11034 | 375 | 2452 | 6.54 |
| `exp2f` | 9269 | 316 | 2212 | 6.99 |
| `expm1f` | 12310 | 12310 | 6768 | 0.55 |
| `logf` | 12367 | 2210 | 2426 | 1.10 |
| `log2f` | 10547 | 583 | 2447 | 4.20 |
| `log10f` | 12365 | 12365 | 2686 | 0.22 |
| `log1pf` | 12789 | 12789 | 7090 | 0.55 |
| `powf` | 22158 | 22158 | 5462 | 0.25 |
| `sinf` | 18350 | 4729 | 3059 | 0.65 |
| `cosf` | 18281 | 3742 | 3077 | 0.82 |
| `tanf` | 36223 | 5140 | 4024 | 0.78 |
| `asinf` | 23011 | 23011 | 13659 | 0.59 |
| `acosf` | 23828 | 23828 | 14158 | 0.59 |
| `atanf` | 17750 | 17750 | 10464 | 0.59 |
| `atan2f` | 18422 | 18422 | 10899 | 0.59 |
| `sinhf` | 15310 | 15310 | 8735 | 0.57 |
| `coshf` | 15181 | 15181 | 8662 | 0.57 |
| `tanhf` | 15977 | 15977 | 9115 | 0.57 |
| `asinhf` | 19353 | 19353 | 11011 | 0.57 |
| `acoshf` | 19926 | 19926 | 11303 | 0.57 |
| `atanhf` | 15868 | 15868 | 8951 | 0.56 |
| `erff` | 141024 | 2967 | 10570 | 3.56 |
| `erfcf` | 119604 | 2874 | 2770 | 0.96 |
| `lgammaf` | 36729 | 36729 | 20889 | 0.57 |
| `tgammaf` | 55326 | 55326 | 31220 | 0.56 |
| `fmodf` | 273 | 273 | 230 | 0.84 |
| `floorf` | 105 | 105 | 70.7 | 0.67 |
| `roundf` | 103 | 103 | 68.7 | 0.66 |
| `sqrt` | 4208 | 4208 | 4075 | 0.97 |
| `cbrt` | 14545 | 14545 | 14512 | 1.00 |
| `hypot` | 14812 | 14812 | 14406 | 0.97 |
| `exp` | 10356 | 10356 | 3954 | 0.38 |
| `exp2` | 14172 | 14172 | 3335 | 0.24 |
| `expm1` | 10978 | 10978 | 10749 | 0.98 |
| `log` | 17003 | 17003 | 4335 | 0.25 |
| `log2` | 21459 | 21459 | 5211 | 0.24 |
| `log10` | 21106 | 21106 | 20900 | 0.99 |
| `log1p` | 18064 | 18064 | 17859 | 0.99 |
| `pow` | 31562 | 31562 | 12178 | 0.39 |
| `sin` | 17710 | 17710 | 17482 | 0.99 |
| `cos` | 17700 | 17700 | 17448 | 0.99 |
| `tan` | 32504 | 32490 | 32298 | 0.99 |
| `asin` | 34679 | 34665 | 34315 | 0.99 |
| `acos` | 35771 | 35757 | 35425 | 0.99 |
| `atan` | 21229 | 21215 | 20980 | 0.99 |
| `atan2` | 22656 | 22642 | 22258 | 0.98 |
| `sinh` | 17881 | 17881 | 17626 | 0.99 |
| `cosh` | 16736 | 16736 | 16481 | 0.98 |
| `tanh` | 20206 | 20192 | 19974 | 0.99 |
| `asinh` | 39824 | 39824 | 39448 | 0.99 |
| `acosh` | 34288 | 34288 | 33915 | 0.99 |
| `atanh` | 25888 | 25874 | 25673 | 0.99 |
| `erf` | 40695 | 40695 | 25491 | 0.63 |
| `erfc` | 112996 | 112996 | 112679 | 1.00 |
| `lgamma` | 47161 | 47123 | 46702 | 0.99 |
| `tgamma` | 64810 | 64762 | 64248 | 0.99 |
| `fmod` | 486 | 486 | 464 | 0.95 |
| `floor` | 212 | 212 | 204 | 0.96 |
| `round` | 210 | 210 | 202 | 0.96 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 90.3 | 90.3 | 50.6 | 0.56 |
| `fsub` | 84.8 | 84.8 | 51.6 | 0.61 |
| `fmul` | 109 | 109 | 56.3 | 0.52 |
| `fdiv` | 292 | 292 | 292 | 1.00 |
| `fcmp` | 44.4 | 44.4 | 24.2 | 0.54 |
| `dadd` | 107 | 107 | 107 | 1.00 |
| `dsub` | 116 | 116 | 116 | 1.00 |
| `dmul` | 264 | 264 | 264 | 1.00 |
| `ddiv` | 844 | 844 | 844 | 1.00 |
| `dcmp` | 66.9 | 66.9 | 24.2 | 0.36 |
| `f2d` | 32.4 | 32.4 | 32.4 | 1.00 |
| `d2f` | 51.5 | 51.5 | 51.5 | 1.00 |
| `f2i` | 24.9 | 24.9 | 24.9 | 1.00 |
| `i2f` | 76.6 | 76.6 | 76.6 | 1.00 |
| `d2i` | 38.4 | 38.4 | 38.4 | 1.00 |
| `i2d` | 61.6 | 61.6 | 61.6 | 1.00 |
| `f2l` | 689 | 689 | 580 | 0.84 |
| `l2f` | 525 | 525 | 525 | 1.00 |
| `d2l` | 696 | 696 | 568 | 0.81 |
| `l2d` | 473 | 473 | 473 | 1.00 |

### armv7e-m

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 239 of 603 | 0.95 | `memchr`, `memset`, `strlen` |
| stdio | 20 of 23 | 0.99 | `__aeabi_d2uiz`, `__aeabi_dadd`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `memchr`, `memset`, `strlen` |
| strto | 11 of 21 | 0.70 | `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `memchr`, `memset` |
| malloc | 1 of 9 | 1.02 | `memchr`, `memset` |
| qsort | 0 of 8 | — | `memchr`, `memset` |
| math | 80 of 124 | 0.81 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `__gedf2`, `__ledf2`, `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `memchr`, `memset`, `pow`, `powf`, `sinf` |
| softfloat | 12 of 20 | 0.47 | `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_dcmplt`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_f2lz`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_l2f`, `__aeabi_ui2d`, `__aeabi_ul2f`, `memchr`, `memset` |

#### string

Aligned case per size: size flavour / speed flavour / AOR.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memset` | 30 / 18 / 19 | 33 / 35 / 29 | 34 / 35 / 38 | 46 / 39 / 40 | 94 / 57 / 49 | 286 / 81 / 85 | 1054 / 177 / 229 |
| `memchr` | 29 / 29 / 23 | 36 / 36 / 27 | 65 / 65 / 42 | 95 / 95 / 63 | 215 / 215 / 123 | 695 / 695 / 363 | 2615 / 2615 / 1323 |
| `strlen` | 35 / 35 / 42 | 42 / 42 / 42 | 40 / 40 / 42 | 55 / 55 / 56 | 115 / 115 / 104 | 355 / 355 / 290 | 1315 / 1315 / 1034 |
| `strcpy` | 65 / 62 / 69 | 76 / 69 / 69 | 75 / 69 / 71 | 105 / 92 / 93 | 225 / 164 / 153 | 705 / 428 / 363 | 2625 / 1484 / 1203 |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `expf` | 160 | 160 | 681 | 4.25 |
| `exp2f` | 141 | 141 | 632 | 4.47 |
| `logf` | 165 | 165 | 649 | 3.94 |
| `log2f` | 154 | 154 | 654 | 4.25 |
| `log10f` | 165 | 165 | 698 | 4.24 |
| `powf` | 367 | 367 | 1478 | 4.03 |
| `sinf` | 575 | 575 | 756 | 1.31 |
| `cosf` | 567 | 567 | 757 | 1.33 |
| `erff` | 1383 | 1383 | 550 | 0.40 |
| `sqrt` | 2009 | 2009 | 1884 | 0.94 |
| `cbrt` | 6937 | 6937 | 2727 | 0.39 |
| `hypot` | 6520 | 6520 | 5289 | 0.81 |
| `exp` | 4185 | 4185 | 1090 | 0.26 |
| `exp2` | 5775 | 5775 | 934 | 0.16 |
| `expm1` | 4481 | 4481 | 3561 | 0.79 |
| `log` | 7530 | 7530 | 1245 | 0.17 |
| `log2` | 9231 | 9231 | 1514 | 0.16 |
| `log10` | 9093 | 9093 | 6522 | 0.72 |
| `log1p` | 8079 | 8079 | 5750 | 0.71 |
| `pow` | 13390 | 13390 | 3576 | 0.27 |
| `sin` | 6876 | 6876 | 5660 | 0.82 |
| `cos` | 6825 | 6825 | 5654 | 0.83 |
| `tan` | 12927 | 12927 | 9716 | 0.75 |
| `asin` | 15803 | 15803 | 11297 | 0.71 |
| `acos` | 16391 | 16391 | 11823 | 0.72 |
| `atan` | 9740 | 9740 | 6552 | 0.67 |
| `atan2` | 10501 | 10501 | 7020 | 0.67 |
| `sinh` | 7566 | 7566 | 5441 | 0.72 |
| `cosh` | 7119 | 7119 | 5073 | 0.71 |
| `tanh` | 8538 | 8538 | 6189 | 0.72 |
| `asinh` | 18322 | 18322 | 13184 | 0.72 |
| `acosh` | 15690 | 15690 | 11696 | 0.75 |
| `atanh` | 11882 | 11882 | 7823 | 0.66 |
| `erf` | 16960 | 16960 | 8263 | 0.49 |
| `erfc` | 51736 | 51736 | 31472 | 0.61 |
| `lgamma` | 21092 | 21092 | 14969 | 0.71 |
| `tgamma` | 28717 | 28717 | 20221 | 0.70 |
| `fmod` | 280 | 280 | 250 | 0.89 |
| `floor` | 119 | 119 | 86.1 | 0.72 |
| `round` | 117 | 117 | 84.1 | 0.72 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fsub` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fmul` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fdiv` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fcmp` | 10.8 | 10.8 | 10.8 | 1.00 |
| `dadd` | 63.9 | 63.9 | 46.2 | 0.72 |
| `dsub` | 67.2 | 67.2 | 46.3 | 0.69 |
| `dmul` | 57.6 | 57.6 | 54.9 | 0.95 |
| `ddiv` | 583 | 583 | 81.9 | 0.14 |
| `dcmp` | 48.9 | 48.9 | 20.2 | 0.41 |
| `f2d` | 15.9 | 15.9 | 15.9 | 1.00 |
| `d2f` | 19.9 | 19.9 | 17.9 | 0.90 |
| `f2i` | 7.8 | 7.8 | 7.8 | 1.00 |
| `i2f` | 7.8 | 7.8 | 7.8 | 1.00 |
| `d2i` | 21.9 | 21.9 | 20.9 | 0.95 |
| `i2d` | 40.9 | 40.9 | 18.9 | 0.46 |
| `f2l` | 141 | 141 | 20.9 | 0.15 |
| `l2f` | 32.9 | 32.9 | 27.3 | 0.83 |
| `d2l` | 178 | 178 | 25.9 | 0.15 |
| `l2d` | 48.6 | 48.6 | 29.6 | 0.61 |

### armv7e-m-dp

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 239 of 603 | 0.95 | `memchr`, `memset`, `strlen` |
| stdio | 19 of 23 | 1.00 | `memchr`, `memset`, `strlen` |
| strto | 11 of 21 | 1.00 | `__aeabi_d2lz`, `memchr`, `memset` |
| malloc | 1 of 9 | 1.02 | `memchr`, `memset` |
| qsort | 0 of 8 | — | `memchr`, `memset` |
| math | 80 of 124 | 0.61 | `__aeabi_d2lz`, `__aeabi_l2d`, `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `memchr`, `memset`, `pow`, `powf`, `sinf` |
| softfloat | 4 of 20 | 0.79 | `__aeabi_d2lz`, `__aeabi_f2lz`, `__aeabi_l2d`, `__aeabi_l2f`, `__aeabi_ul2f`, `memchr`, `memset` |

#### string

Aligned case per size: size flavour / speed flavour / AOR.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memset` | 30 / 18 / 19 | 33 / 35 / 29 | 34 / 35 / 38 | 46 / 39 / 40 | 94 / 57 / 49 | 286 / 81 / 85 | 1054 / 177 / 229 |
| `memchr` | 29 / 29 / 23 | 36 / 36 / 27 | 65 / 65 / 42 | 95 / 95 / 63 | 215 / 215 / 123 | 695 / 695 / 363 | 2615 / 2615 / 1323 |
| `strlen` | 35 / 35 / 42 | 42 / 42 / 42 | 40 / 40 / 42 | 55 / 55 / 56 | 115 / 115 / 104 | 355 / 355 / 290 | 1315 / 1315 / 1034 |
| `strcpy` | 65 / 62 / 69 | 76 / 69 / 69 | 75 / 69 / 71 | 105 / 92 / 93 | 225 / 164 / 153 | 705 / 428 / 363 | 2625 / 1484 / 1203 |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `expf` | 172 | 172 | 42.3 | 0.25 |
| `exp2f` | 186 | 186 | 39.3 | 0.21 |
| `logf` | 153 | 153 | 46.3 | 0.30 |
| `log2f` | 203 | 203 | 48.3 | 0.24 |
| `log10f` | 164 | 164 | 50.3 | 0.31 |
| `powf` | 396 | 396 | 88.4 | 0.22 |
| `sinf` | 181 | 181 | 50.5 | 0.28 |
| `cosf` | 171 | 171 | 47.2 | 0.28 |
| `erff` | 421 | 421 | 67.1 | 0.16 |
| `sqrt` | 11.6 | 11.6 | 11.6 | 0.99 |
| `cbrt` | 134 | 134 | 134 | 1.00 |
| `hypot` | 138 | 138 | 138 | 1.00 |
| `exp` | 169 | 169 | 55.6 | 0.33 |
| `exp2` | 183 | 183 | 51.6 | 0.28 |
| `expm1` | 172 | 172 | 172 | 1.00 |
| `log` | 150 | 150 | 67.6 | 0.45 |
| `log2` | 200 | 200 | 72.6 | 0.36 |
| `log10` | 161 | 161 | 161 | 1.00 |
| `log1p` | 187 | 187 | 187 | 1.00 |
| `pow` | 392 | 392 | 135 | 0.34 |
| `sin` | 165 | 165 | 165 | 1.00 |
| `cos` | 167 | 167 | 167 | 1.00 |
| `tan` | 207 | 207 | 207 | 1.00 |
| `asin` | 220 | 220 | 220 | 1.00 |
| `acos` | 228 | 228 | 228 | 1.00 |
| `atan` | 195 | 195 | 195 | 1.00 |
| `atan2` | 360 | 360 | 360 | 1.00 |
| `sinh` | 200 | 200 | 200 | 1.00 |
| `cosh` | 192 | 192 | 192 | 1.00 |
| `tanh` | 202 | 202 | 202 | 1.00 |
| `asinh` | 287 | 287 | 287 | 1.00 |
| `acosh` | 246 | 246 | 246 | 1.00 |
| `atanh` | 217 | 217 | 217 | 1.00 |
| `erf` | 418 | 418 | 69.2 | 0.17 |
| `erfc` | 783 | 783 | 783 | 1.00 |
| `lgamma` | 499 | 499 | 499 | 1.00 |
| `tgamma` | 719 | 719 | 718 | 1.00 |
| `fmod` | 178 | 178 | 178 | 1.00 |
| `floor` | 11.8 | 11.8 | 11.8 | 0.99 |
| `round` | 11.8 | 11.8 | 11.8 | 0.99 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fsub` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fmul` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fdiv` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fcmp` | 10.8 | 10.8 | 10.8 | 1.00 |
| `dadd` | 9.9 | 9.9 | 9.9 | 1.00 |
| `dsub` | 9.9 | 9.9 | 9.9 | 1.00 |
| `dmul` | 9.9 | 9.9 | 9.9 | 1.00 |
| `ddiv` | 9.9 | 9.9 | 9.9 | 1.00 |
| `dcmp` | 10.8 | 10.8 | 10.8 | 1.00 |
| `f2d` | 8.9 | 8.9 | 8.9 | 1.00 |
| `d2f` | 6.8 | 6.8 | 6.8 | 1.00 |
| `f2i` | 7.8 | 7.8 | 7.8 | 1.00 |
| `i2f` | 7.8 | 7.8 | 7.8 | 1.00 |
| `d2i` | 7.8 | 7.8 | 7.8 | 1.00 |
| `i2d` | 9.9 | 9.9 | 9.9 | 1.00 |
| `f2l` | 25.9 | 25.9 | 20.9 | 0.81 |
| `l2f` | 32.9 | 32.9 | 27.3 | 0.83 |
| `d2l` | 26.9 | 26.9 | 25.9 | 0.96 |
| `l2d` | 48.6 | 48.6 | 29.6 | 0.61 |

### armv8-m.main

Instructions per operation: rtoskit-libc's size and speed flavours, and the speed flavour with AOR's routines linked in. The ratio is AOR against the speed flavour, as a geometric mean over the cases whose count changed.

| Program | Cases changed | AOR / speed (geometric mean) | AOR symbols linked |
|---|---:|---:|---|
| string | 239 of 603 | 0.95 | `memchr`, `memset`, `strlen` |
| stdio | 20 of 23 | 0.99 | `__aeabi_d2uiz`, `__aeabi_dadd`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `memchr`, `memset`, `strlen` |
| strto | 11 of 21 | 0.70 | `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `memchr`, `memset` |
| malloc | 1 of 9 | 1.02 | `memchr`, `memset` |
| qsort | 0 of 8 | — | `memchr`, `memset` |
| math | 80 of 124 | 0.81 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `__gedf2`, `__ledf2`, `cosf`, `erf`, `erff`, `exp`, `exp2`, `exp2f`, `expf`, `log`, `log10f`, `log2`, `log2f`, `logf`, `memchr`, `memset`, `pow`, `powf`, `sinf` |
| softfloat | 12 of 20 | 0.47 | `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_d2lz`, `__aeabi_dadd`, `__aeabi_dcmplt`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_f2lz`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_l2f`, `__aeabi_ui2d`, `__aeabi_ul2f`, `memchr`, `memset` |

#### string

Aligned case per size: size flavour / speed flavour / AOR.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memset` | 30 / 18 / 19 | 33 / 35 / 29 | 34 / 35 / 38 | 46 / 39 / 40 | 94 / 57 / 49 | 286 / 81 / 85 | 1054 / 177 / 229 |
| `memchr` | 29 / 29 / 23 | 36 / 36 / 27 | 65 / 65 / 42 | 95 / 95 / 63 | 215 / 215 / 123 | 695 / 695 / 363 | 2615 / 2615 / 1323 |
| `strlen` | 35 / 35 / 42 | 42 / 42 / 42 | 40 / 40 / 42 | 55 / 55 / 56 | 115 / 115 / 104 | 355 / 355 / 290 | 1315 / 1315 / 1034 |
| `strcpy` | 65 / 62 / 69 | 76 / 69 / 69 | 75 / 69 / 71 | 105 / 92 / 93 | 225 / 164 / 153 | 705 / 428 / 363 | 2625 / 1484 / 1203 |

#### math

Throughput per call, for the functions whose count changed (AOR's own, or ours through AOR's soft-float).

| Function | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `expf` | 160 | 160 | 681 | 4.25 |
| `exp2f` | 141 | 141 | 632 | 4.47 |
| `logf` | 165 | 165 | 649 | 3.94 |
| `log2f` | 154 | 154 | 654 | 4.25 |
| `log10f` | 165 | 165 | 698 | 4.24 |
| `powf` | 353 | 353 | 1478 | 4.19 |
| `sinf` | 574 | 574 | 756 | 1.32 |
| `cosf` | 566 | 566 | 757 | 1.34 |
| `erff` | 1383 | 1383 | 550 | 0.40 |
| `sqrt` | 2009 | 2009 | 1884 | 0.94 |
| `cbrt` | 6937 | 6937 | 2727 | 0.39 |
| `hypot` | 6520 | 6520 | 5289 | 0.81 |
| `exp` | 4185 | 4185 | 1090 | 0.26 |
| `exp2` | 5776 | 5776 | 934 | 0.16 |
| `expm1` | 4481 | 4481 | 3561 | 0.79 |
| `log` | 7530 | 7530 | 1245 | 0.17 |
| `log2` | 9231 | 9231 | 1514 | 0.16 |
| `log10` | 9093 | 9093 | 6522 | 0.72 |
| `log1p` | 8079 | 8079 | 5750 | 0.71 |
| `pow` | 13391 | 13391 | 3576 | 0.27 |
| `sin` | 6876 | 6876 | 5660 | 0.82 |
| `cos` | 6825 | 6825 | 5654 | 0.83 |
| `tan` | 12927 | 12927 | 9716 | 0.75 |
| `asin` | 15803 | 15803 | 11297 | 0.71 |
| `acos` | 16391 | 16391 | 11823 | 0.72 |
| `atan` | 9740 | 9740 | 6552 | 0.67 |
| `atan2` | 10501 | 10501 | 7020 | 0.67 |
| `sinh` | 7566 | 7566 | 5441 | 0.72 |
| `cosh` | 7119 | 7119 | 5073 | 0.71 |
| `tanh` | 8538 | 8538 | 6189 | 0.72 |
| `asinh` | 18322 | 18322 | 13184 | 0.72 |
| `acosh` | 15690 | 15690 | 11696 | 0.75 |
| `atanh` | 11882 | 11882 | 7823 | 0.66 |
| `erf` | 16961 | 16961 | 8251 | 0.49 |
| `erfc` | 51737 | 51737 | 31472 | 0.61 |
| `lgamma` | 21092 | 21092 | 14969 | 0.71 |
| `tgamma` | 28717 | 28717 | 20221 | 0.70 |
| `fmod` | 281 | 281 | 251 | 0.89 |
| `floor` | 119 | 119 | 86.1 | 0.72 |
| `round` | 117 | 117 | 84.1 | 0.72 |

#### softfloat

Per operation.

| Operation | size | speed | AOR | AOR / speed |
|---|---:|---:|---:|---:|
| `fadd` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fsub` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fmul` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fdiv` | 7.8 | 7.8 | 7.8 | 1.00 |
| `fcmp` | 10.8 | 10.8 | 10.8 | 1.00 |
| `dadd` | 63.9 | 63.9 | 46.2 | 0.72 |
| `dsub` | 67.2 | 67.2 | 46.3 | 0.69 |
| `dmul` | 57.6 | 57.6 | 54.9 | 0.95 |
| `ddiv` | 583 | 583 | 81.9 | 0.14 |
| `dcmp` | 48.9 | 48.9 | 20.2 | 0.41 |
| `f2d` | 15.9 | 15.9 | 15.9 | 1.00 |
| `d2f` | 19.9 | 19.9 | 17.9 | 0.90 |
| `f2i` | 7.8 | 7.8 | 7.8 | 1.00 |
| `i2f` | 7.8 | 7.8 | 7.8 | 1.00 |
| `d2i` | 21.9 | 21.9 | 20.9 | 0.95 |
| `i2d` | 40.9 | 40.9 | 18.9 | 0.46 |
| `f2l` | 141 | 141 | 20.9 | 0.15 |
| `l2f` | 32.9 | 32.9 | 27.3 | 0.83 |
| `d2l` | 178 | 178 | 25.9 | 0.15 |
| `l2d` | 48.6 | 48.6 | 29.6 | 0.61 |

