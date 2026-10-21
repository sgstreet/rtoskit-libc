# Phase 3 performance report

The end of phase 3 (P3.06, PLAN D89).

1. **Against phase 2:** the instruction counts of both flavours compared with the phase 2 baseline (`docs/perf/baseline-*.json` at commit 75cd261, taken in P3.01 before any phase 3 change).
2. **Size budgets:** the images of both flavours against the phase 2 size lock.
3. **Against picolibc 1.8.6 and newlib-nano 4.4:** the same benchmark programs, for context.

The step-by-step history is in `docs/perf/<arch>.md` and `docs/size/<arch>.md`.

How the figures were made:
- **Counting:** every figure is guest instructions per operation, counted by the `tools/qemu-plugins/bench-roi.c` plugin under qemu 8.2.2 (D72). x86-64 runs under qemu-x86_64 for the counts.
- **What the counts miss:** they are exact and repeatable, but blind to cycles, pipelines and memory. On the Cortex-M7 in particular they cannot show dual issue (D84).
- **Ratios:** each ratio is a geometric mean over the cases the builds share. The math programs count throughput cases only, since the latency counts are the same under qemu.

## Phase 2 → phase 3

The ratio of today's count to the phase 2 count (below 1 is faster).
- The size flavour is the `release` build. It changed only where a change cost no code, or saved some.
- The speed flavour (D73) is new in phase 3, so its ratios compare it with the phase 2 size build.

| Arch | Program | Cases | Size flavour | Speed flavour | Best case (speed) |
|---|---|---:|---:|---:|---|
| x86_64 | string | 603 | 1.00 | 0.52 | `memcpy/1024/0:1` 6184 → 303 |
| x86_64 | stdio | 23 | 1.00 | 0.88 | `fputs/line` 484 → 198 |
| x86_64 | strto | 21 | 1.00 | 0.26 | `strtod/exp-large` 219167 → 949 |
| x86_64 | malloc | 9 | 0.81 | 0.77 | `calloc/256` 226 → 124 |
| x86_64 | malloc-tiny | 9 | 0.86 | 0.81 | `calloc/256` 235 → 133 |
| x86_64 | qsort | 8 | 1.00 | 0.98 | `qsort/strings/256` 706 → 617 |
| x86_64 | math | 62 | 1.00 | 1.00 | `acos/throughput` 504 → 504 |
| x86_64 | threads | 7 | 1.00 | 1.00 | `call_once/done` 22 → 22 |
| armv6-m | string | 603 | 1.00 | 0.89 | `memset/1024/0` 1054 → 169 |
| armv6-m | stdio | 23 | 1.00 | 0.87 | `snprintf/%lld` 11027 → 3702 |
| armv6-m | strto | 21 | 1.00 | 0.13 | `strtod/subnormal` 2040498 → 3849 |
| armv6-m | malloc | 9 | 0.96 | 0.93 | `calloc/256` 378 → 262 |
| armv6-m | malloc-tiny | 9 | 0.97 | 0.94 | `calloc/256` 386 → 270 |
| armv6-m | qsort | 8 | 1.00 | 1.00 | `bsearch/1024` 167 → 167 |
| armv6-m | math | 62 | 0.95 | 0.65 | `erff/throughput` 151747 → 2967 |
| armv6-m | threads | 7 | 1.00 | 1.00 | `call_once/done` 22 → 22 |
| armv6s-m | string | 603 | 1.00 | 0.89 | `memset/1024/0` 1054 → 169 |
| armv6s-m | stdio | 23 | 1.00 | 0.87 | `snprintf/%lld` 11027 → 3702 |
| armv6s-m | strto | 21 | 1.00 | 0.13 | `strtod/subnormal` 2040498 → 3849 |
| armv6s-m | malloc | 9 | 0.96 | 0.93 | `calloc/256` 378 → 262 |
| armv6s-m | malloc-tiny | 9 | 0.97 | 0.94 | `calloc/256` 386 → 270 |
| armv6s-m | qsort | 8 | 1.00 | 1.00 | `bsearch/1024` 167 → 167 |
| armv6s-m | math | 62 | 0.95 | 0.65 | `erff/throughput` 151747 → 2967 |
| armv6s-m | threads | 7 | 1.00 | 1.00 | `call_once/done` 22 → 22 |
| armv7e-m | string | 603 | 1.00 | 0.70 | `memmove-up/1024/3:2` 5146 → 405 |
| armv7e-m | stdio | 23 | 1.00 | 0.82 | `snprintf/%lld` 7643 → 1353 |
| armv7e-m | strto | 21 | 1.00 | 0.18 | `strtod/exp-large` 176358 → 1314 |
| armv7e-m | malloc | 9 | 0.88 | 0.82 | `calloc/256` 249 → 132 |
| armv7e-m | malloc-tiny | 9 | 0.90 | 0.85 | `calloc/256` 254 → 136 |
| armv7e-m | qsort | 8 | 1.00 | 1.00 | `qsort/strings/256` 595 → 585 |
| armv7e-m | math | 62 | 0.87 | 0.87 | `erff/throughput` 2538 → 1383 |
| armv7e-m | threads | 7 | 1.00 | 1.00 | `call_once/done` 19 → 19 |
| armv7e-m-dp | string | 603 | 1.00 | 0.70 | `memmove-up/1024/3:2` 5146 → 405 |
| armv7e-m-dp | stdio | 23 | 1.00 | 0.82 | `snprintf/%lld` 7643 → 1353 |
| armv7e-m-dp | strto | 21 | 1.00 | 0.15 | `strtod/exp-large` 176132 → 1088 |
| armv7e-m-dp | malloc | 9 | 0.88 | 0.82 | `calloc/256` 249 → 132 |
| armv7e-m-dp | malloc-tiny | 9 | 0.90 | 0.85 | `calloc/256` 254 → 136 |
| armv7e-m-dp | qsort | 8 | 1.00 | 1.00 | `qsort/strings/256` 595 → 585 |
| armv7e-m-dp | math | 62 | 0.84 | 0.84 | `asin/throughput` 336 → 220 |
| armv7e-m-dp | threads | 7 | 1.00 | 1.00 | `call_once/done` 19 → 19 |
| armv8-m.main | string | 603 | 1.00 | 0.70 | `memmove-up/1024/3:2` 5146 → 405 |
| armv8-m.main | stdio | 23 | 1.00 | 0.83 | `snprintf/%lld` 7643 → 1353 |
| armv8-m.main | strto | 21 | 1.00 | 0.18 | `strtod/exp-large` 176378 → 1313 |
| armv8-m.main | malloc | 9 | 0.87 | 0.82 | `calloc/256` 247 → 130 |
| armv8-m.main | malloc-tiny | 9 | 0.90 | 0.85 | `calloc/256` 252 → 134 |
| armv8-m.main | qsort | 8 | 1.00 | 1.00 | `qsort/strings/256` 595 → 585 |
| armv8-m.main | math | 62 | 0.87 | 0.87 | `erff/throughput` 2538 → 1383 |
| armv8-m.main | threads | 7 | 1.00 | 1.00 | `call_once/done` 18 → 18 |

Where the gains came from:
- **string:** the Thumb-2 and Armv6-M memory and string kernels, and the x86-64 SSE2 kernels (D73–D76).
- **strto:** integer conversions without 64-bit division (D77), and the `strtod` fast path for up to 19 digits (D78).
- **malloc:**
  - the lock word's fast path in the port (D79), in both flavours;
  - the allocators themselves were measured and kept (D80).
- **math on FPv4/FPv5:** the float-float and double-double exact products by fused multiply-add (D82, D83), in both flavours.
- **math on Armv6-M:**
  - integer `fmul`, `fdiv` and `sqrtf` in both flavours (D85);
  - fixed-point `expf`, `exp2f`, `logf`, `log2f`, `sinf`, `cosf`, `tanf`, `erff` and `erfcf` in the speed flavour (D86–D88);
  - each keeps the float-float code for the results it cannot round with certainty, so the results are the size flavour's or closer.
- **qsort and threads:** unchanged. No step targeted them, and their counts are dominated by the comparison callbacks and the futex path.

## Size budgets

`size-report budget` passes for both flavours on every Arm board. The size flavour may not grow beyond the phase 2 lock (+0 %, D71). The speed flavour may grow by +10 %, or +20 % for `math-float` and `math-mix` (D87). Code (text + rodata) per image against the phase 2 lock:

| Arch | Program | Phase 2 | Size flavour | Speed flavour |
|---|---|---:|---:|---:|
| armv6-m | float-exact | 3908 | 3744 (-4.2 %) | 3744 (-4.2 %) |
| armv6-m | hello-printf-float | 10374 | 10366 (-0.1 %) | 10806 (+4.2 %) |
| armv6-m | hello-printf-int | 7769 | 7761 (-0.1 %) | 8201 (+5.6 %) |
| armv6-m | hello-printf-int-minimal | 4719 | 4711 (-0.2 %) | 4835 (+2.5 %) |
| armv6-m | hello-printf-int-nofloat | 4987 | 4979 (-0.2 %) | 5307 (+6.4 %) |
| armv6-m | hello-puts | 2163 | 2155 (-0.4 %) | 2243 (+3.7 %) |
| armv6-m | malloc-heavy | 2214 | 2206 (-0.4 %) | 2330 (+5.2 %) |
| armv6-m | malloc-heavy-tiny | 2134 | 2126 (-0.4 %) | 2250 (+5.4 %) |
| armv6-m | math-float | 9640 | 9036 (-6.3 %) | 11460 (+18.9 %) |
| armv6-m | math-mix | 34116 | 33584 (-1.6 %) | 36060 (+5.7 %) |
| armv7e-m | float-exact | 2048 | 2048 (+0.0 %) | 2048 (+0.0 %) |
| armv7e-m | hello-printf-float | 9170 | 9154 (-0.2 %) | 9682 (+5.6 %) |
| armv7e-m | hello-printf-int | 7185 | 7169 (-0.2 %) | 7697 (+7.1 %) |
| armv7e-m | hello-printf-int-minimal | 4231 | 4215 (-0.4 %) | 4395 (+3.9 %) |
| armv7e-m | hello-printf-int-nofloat | 4471 | 4455 (-0.4 %) | 4821 (+7.8 %) |
| armv7e-m | hello-puts | 1903 | 1887 (-0.8 %) | 2007 (+5.5 %) |
| armv7e-m | malloc-heavy | 1934 | 1914 (-1.0 %) | 2094 (+8.3 %) |
| armv7e-m | malloc-heavy-tiny | 1874 | 1854 (-1.1 %) | 2034 (+8.5 %) |
| armv7e-m | math-float | 5282 | 4818 (-8.8 %) | 4818 (-8.8 %) |
| armv7e-m | math-mix | 27312 | 26844 (-1.7 %) | 26908 (-1.5 %) |
| armv7e-m-dp | float-exact | 1688 | 1688 (+0.0 %) | 1688 (+0.0 %) |
| armv7e-m-dp | hello-printf-float | 7244 | 7228 (-0.2 %) | 7756 (+7.1 %) |
| armv7e-m-dp | hello-printf-int | 7220 | 7204 (-0.2 %) | 7732 (+7.1 %) |
| armv7e-m-dp | hello-printf-int-minimal | 4231 | 4215 (-0.4 %) | 4395 (+3.9 %) |
| armv7e-m-dp | hello-printf-int-nofloat | 4471 | 4455 (-0.4 %) | 4821 (+7.8 %) |
| armv7e-m-dp | hello-puts | 1903 | 1887 (-0.8 %) | 2007 (+5.5 %) |
| armv7e-m-dp | malloc-heavy | 1934 | 1914 (-1.0 %) | 2094 (+8.3 %) |
| armv7e-m-dp | malloc-heavy-tiny | 1874 | 1854 (-1.1 %) | 2034 (+8.5 %) |
| armv7e-m-dp | math-float | 9494 | 9078 (-4.4 %) | 9134 (-3.8 %) |
| armv7e-m-dp | math-mix | 12302 | 11662 (-5.2 %) | 11726 (-4.7 %) |
| armv8-m.main | float-exact | 1612 | 1612 (+0.0 %) | 1612 (+0.0 %) |
| armv8-m.main | hello-printf-float | 9150 | 9138 (-0.1 %) | 9658 (+5.6 %) |
| armv8-m.main | hello-printf-int | 7165 | 7153 (-0.2 %) | 7673 (+7.1 %) |
| armv8-m.main | hello-printf-int-minimal | 4203 | 4191 (-0.3 %) | 4371 (+4.0 %) |
| armv8-m.main | hello-printf-int-nofloat | 4439 | 4427 (-0.3 %) | 4789 (+7.9 %) |
| armv8-m.main | hello-puts | 1883 | 1871 (-0.6 %) | 1991 (+5.7 %) |
| armv8-m.main | malloc-heavy | 1914 | 1898 (-0.8 %) | 2078 (+8.6 %) |
| armv8-m.main | malloc-heavy-tiny | 1854 | 1838 (-0.9 %) | 2018 (+8.8 %) |
| armv8-m.main | math-float | 5214 | 4750 (-8.9 %) | 4750 (-8.9 %) |
| armv8-m.main | math-mix | 27232 | 26772 (-1.7 %) | 26828 (-1.5 %) |

armv6s-m is the same as armv6-m.
- The size flavour is smaller than at the end of phase 2 in every image, by 0.1–8.9 %:
  - the lock word (D79);
  - the FMA products (D82, D83);
  - Armv6-M's integer `fmul`/`fdiv`/`sqrtf` (D85).
- The speed flavour's largest image growth is Armv6-M's `math-float`, at +18.9 % (D87). The rest stay within +8.8 %.
- RAM is unchanged or smaller in every image.

## Against picolibc and newlib-nano

`tools/scripts/bench-runner compare` builds the same `test/bench` programs against picolibc 1.8.6 and newlib-nano 4.4.0:
- **The libraries:** the Ubuntu 24.04 packages that `size-report compare` pins by SHA-256 (D51).
- **The build:** the same compiler and flags. Each library has a small shim in `tools/bench-compare`: Linux `write`, `open`, `exit`, a static heap, and picolibc's `.bss` and TLS setup. newlib-nano links `_printf_float` and `_scanf_float`.
- **The measurement:** the same qemu plugin counts every program.
- **What is left out:** `malloc-tiny` is rtoskit's own allocator, and `threads` needs `<threads.h>`, which neither library has.

This is context, not a gate:
- **Accuracy:** the libraries do not compute the same thing. rtoskit's float and double functions are correctly rounded but for rare cases: within 0.52 ulp, with at most 1 in 10,000 not correctly rounded (D57, D47). picolibc and newlib use fdlibm-style code with errors of up to about 1 ulp.
- **Formatting:** rtoskit formats `%f`/`%g` exactly through arbitrary precision (D39).

Each table gives:
- the geometric mean of every common case, as a ratio to rtoskit's speed flavour (above 1 is slower than rtoskit's speed flavour);
- a few cases in instructions per operation.

### armv6-m

| Program | Case | rtoskit (size) | rtoskit (speed) | picolibc | newlib-nano |
|---|---|---:|---:|---:|---:|
| string | geometric mean (603 cases) | 1.13 | 1.00 | 1.31 | 1.15 |
| | `memcpy/16/0:0` | 54.0 | 40.0 | 119 | 65.0 |
| | `memcpy/1024/0:0` | 1314 | 292 | 6167 | 884 |
| | `memset/1024/0` | 1054 | 169 | 5141 | 503 |
| | `strlen/64/0` | 115 | 115 | 279 | 146 |
| | `strcmp/64/0:0` | 541 | 478 | 537 | 167 |
| stdio | geometric mean (23 cases) | 1.14 | 1.00 | 1.67 | 1.12 |
| | `snprintf/%d` | 1638 | 1622 | 2151 | 1222 |
| | `snprintf/%s` | 451 | 428 | 650 | 669 |
| | `snprintf/%f` | 13287 | 13202 | 15716 | 6331 |
| | `snprintf/%g` | 14849 | 14761 | 13075 | 6444 |
| | `fprintf/mix` | 4134 | 4077 | 5802 | 4335 |
| strto | geometric mean (21 cases) | 7.52 | 1.00 | 1.20 | 1.47 |
| | `strtol/int` | 1501 | 422 | 513 | 535 |
| | `strtod/short` | 3352 | 2459 | 2625 | 2079 |
| | `strtod/17-digits` | 24062 | 3799 | 3908 | 5320 |
| malloc | geometric mean (9 cases) | 1.04 | 1.00 | 3.87 | 0.36 |
| | `fixed/16` | 224 | 224 | 90.5 | 49.5 |
| | `batch/shuffled` | 247 | 247 | 412 | 75.0 |
| | `realloc/grow` | 225 | 225 | 36095 | 452 |
| qsort | geometric mean (8 cases) | 1.00 | 1.00 | 0.37 | 0.36 |
| | `qsort/random/1024` | 769 | 769 | 313 | 309 |
| math | geometric mean (124 cases) | 1.47 | 1.00 | 0.43 | 0.45 |
| | `sqrtf/throughput` | 461 | 461 | 397 | 718 |
| | `expf/throughput` | 11034 | 375 | 2659 | 2768 |
| | `logf/throughput` | 12367 | 2210 | 2912 | 2982 |
| | `sinf/throughput` | 18350 | 4729 | 2697 | 2706 |
| | `powf/throughput` | 22158 | 22158 | 9269 | 9376 |
| | `erff/throughput` | 141024 | 2967 | 6949 | 6967 |
| | `exp/throughput` | 10356 | 10356 | 5013 | 5156 |
| | `log/throughput` | 17003 | 17003 | 5631 | 5734 |
| | `sin/throughput` | 17710 | 17710 | 5001 | 5003 |
| | `pow/throughput` | 31562 | 31562 | 17110 | 17279 |

### armv7e-m

| Program | Case | rtoskit (size) | rtoskit (speed) | picolibc | newlib-nano |
|---|---|---:|---:|---:|---:|
| string | geometric mean (603 cases) | 1.44 | 1.00 | 1.40 | 1.12 |
| | `memcpy/16/0:0` | 53.0 | 42.0 | 87.0 | 41.0 |
| | `memcpy/1024/0:0` | 1313 | 174 | 4119 | 575 |
| | `memset/1024/0` | 1054 | 177 | 4117 | 366 |
| | `strlen/64/0` | 115 | 115 | 215 | 104 |
| | `strcmp/64/0:0` | 540 | 240 | 410 | 127 |
| stdio | geometric mean (23 cases) | 1.22 | 1.00 | 1.15 | 0.92 |
| | `snprintf/%d` | 475 | 455 | 893 | 506 |
| | `snprintf/%s` | 423 | 395 | 543 | 534 |
| | `snprintf/%f` | 14734 | 14652 | 3993 | 3266 |
| | `snprintf/%g` | 15538 | 15453 | 3646 | 3382 |
| | `fprintf/mix` | 2326 | 2151 | 3506 | 2938 |
| strto | geometric mean (21 cases) | 5.61 | 1.00 | 0.85 | 1.52 |
| | `strtol/int` | 1213 | 305 | 239 | 204 |
| | `strtod/short` | 1962 | 1137 | 821 | 1044 |
| | `strtod/17-digits` | 5376 | 1377 | 1241 | 3130 |
| malloc | geometric mean (9 cases) | 1.07 | 1.00 | 7.21 | 0.68 |
| | `fixed/16` | 92.5 | 92.5 | 76.0 | 40.5 |
| | `batch/shuffled` | 111 | 111 | 335 | 63.7 |
| | `realloc/grow` | 99.2 | 99.2 | 28882 | 315 |
| qsort | geometric mean (8 cases) | 1.00 | 1.00 | 0.38 | 0.41 |
| | `qsort/random/1024` | 640 | 640 | 278 | 277 |
| math | geometric mean (124 cases) | 1.00 | 1.00 | 0.36 | 0.38 |
| | `sqrtf/throughput` | 10.3 | 10.3 | 10.3 | 23.3 |
| | `expf/throughput` | 160 | 160 | 75.9 | 94.9 |
| | `logf/throughput` | 165 | 165 | 84.5 | 85.6 |
| | `sinf/throughput` | 575 | 575 | 101 | 103 |
| | `powf/throughput` | 367 | 367 | 234 | 269 |
| | `erff/throughput` | 1383 | 1383 | 141 | 141 |
| | `exp/throughput` | 4185 | 4185 | 2085 | 2200 |
| | `log/throughput` | 7530 | 7530 | 2207 | 2285 |
| | `sin/throughput` | 6876 | 6876 | 1767 | 1763 |
| | `pow/throughput` | 13390 | 13390 | 7037 | 7205 |

### armv7e-m-dp

| Program | Case | rtoskit (size) | rtoskit (speed) | picolibc | newlib-nano |
|---|---|---:|---:|---:|---:|
| string | geometric mean (603 cases) | 1.44 | 1.00 | 1.40 | 1.12 |
| | `memcpy/16/0:0` | 53.0 | 42.0 | 87.0 | 41.0 |
| | `memcpy/1024/0:0` | 1313 | 174 | 4119 | 575 |
| | `memset/1024/0` | 1054 | 177 | 4117 | 366 |
| | `strlen/64/0` | 115 | 115 | 215 | 104 |
| | `strcmp/64/0:0` | 540 | 240 | 410 | 127 |
| stdio | geometric mean (23 cases) | 1.22 | 1.00 | 1.16 | 0.74 |
| | `snprintf/%d` | 475 | 455 | 893 | 506 |
| | `snprintf/%s` | 423 | 395 | 543 | 534 |
| | `snprintf/%f` | 14737 | 14655 | 3993 | 1057 |
| | `snprintf/%g` | 15541 | 15456 | 3646 | 1189 |
| | `fprintf/mix` | 2326 | 2151 | 3506 | 2938 |
| strto | geometric mean (21 cases) | 6.55 | 1.00 | 0.96 | 1.24 |
| | `strtol/int` | 1213 | 305 | 239 | 204 |
| | `strtod/short` | 1151 | 326 | 618 | 233 |
| | `strtod/17-digits` | 5179 | 1180 | 1038 | 2175 |
| malloc | geometric mean (9 cases) | 1.07 | 1.00 | 7.21 | 0.68 |
| | `fixed/16` | 92.5 | 92.5 | 76.0 | 40.5 |
| | `batch/shuffled` | 111 | 111 | 335 | 63.7 |
| | `realloc/grow` | 99.2 | 99.2 | 28882 | 315 |
| qsort | geometric mean (8 cases) | 1.00 | 1.00 | 0.38 | 0.41 |
| | `qsort/random/1024` | 640 | 640 | 278 | 277 |
| math | geometric mean (124 cases) | 1.00 | 1.00 | 0.52 | 0.48 |
| | `sqrtf/throughput` | 10.3 | 10.3 | 10.3 | 23.3 |
| | `expf/throughput` | 172 | 172 | 75.9 | 43.3 |
| | `logf/throughput` | 153 | 153 | 80.5 | 44.3 |
| | `sinf/throughput` | 181 | 181 | 101 | 49.5 |
| | `powf/throughput` | 396 | 396 | 232 | 89.4 |
| | `erff/throughput` | 421 | 421 | 141 | 121 |
| | `exp/throughput` | 169 | 169 | 69.8 | 56.6 |
| | `log/throughput` | 150 | 150 | 79.8 | 64.6 |
| | `sin/throughput` | 165 | 165 | 102 | 106 |
| | `pow/throughput` | 392 | 392 | 227 | 134 |

### armv8-m.main

| Program | Case | rtoskit (size) | rtoskit (speed) | picolibc | newlib-nano |
|---|---|---:|---:|---:|---:|
| string | geometric mean (603 cases) | 1.44 | 1.00 | 1.49 | 1.19 |
| | `memcpy/16/0:0` | 53.0 | 42.0 | 87.0 | 43.0 |
| | `memcpy/1024/0:0` | 1313 | 174 | 4119 | 607 |
| | `memset/1024/0` | 1054 | 177 | 4117 | 366 |
| | `strlen/64/0` | 115 | 115 | 215 | 154 |
| | `strcmp/64/0:0` | 540 | 240 | 410 | 176 |
| stdio | geometric mean (23 cases) | 1.21 | 1.00 | 1.14 | 0.94 |
| | `snprintf/%d` | 475 | 455 | 893 | 520 |
| | `snprintf/%s` | 423 | 395 | 543 | 535 |
| | `snprintf/%f` | 14733 | 14651 | 3994 | 3276 |
| | `snprintf/%g` | 15537 | 15452 | 3643 | 3393 |
| | `fprintf/mix` | 2323 | 2148 | 3506 | 2946 |
| strto | geometric mean (21 cases) | 5.61 | 1.00 | 0.85 | 1.52 |
| | `strtol/int` | 1213 | 305 | 239 | 211 |
| | `strtod/short` | 1962 | 1137 | 821 | 1040 |
| | `strtod/17-digits` | 5396 | 1376 | 1241 | 3134 |
| malloc | geometric mean (9 cases) | 1.07 | 1.00 | 7.35 | 0.68 |
| | `fixed/16` | 90.5 | 90.5 | 76.0 | 40.5 |
| | `batch/shuffled` | 109 | 109 | 335 | 60.7 |
| | `realloc/grow` | 97.4 | 97.4 | 28882 | 324 |
| qsort | geometric mean (8 cases) | 1.00 | 1.00 | 0.38 | 0.41 |
| | `qsort/random/1024` | 640 | 640 | 278 | 277 |
| math | geometric mean (124 cases) | 1.00 | 1.00 | 0.36 | 0.38 |
| | `sqrtf/throughput` | 10.3 | 10.3 | 10.3 | 23.3 |
| | `expf/throughput` | 160 | 160 | 75.9 | 93.9 |
| | `logf/throughput` | 165 | 165 | 83.5 | 87.6 |
| | `sinf/throughput` | 574 | 574 | 101 | 102 |
| | `powf/throughput` | 353 | 353 | 232 | 273 |
| | `erff/throughput` | 1383 | 1383 | 141 | 139 |
| | `exp/throughput` | 4185 | 4185 | 2085 | 2201 |
| | `log/throughput` | 7530 | 7530 | 2207 | 2285 |
| | `sin/throughput` | 6876 | 6876 | 1767 | 1763 |
| | `pow/throughput` | 13391 | 13391 | 7037 | 7207 |

armv6s-m is the same as armv6-m.

Where rtoskit stands:
- **Faster:**
  - **string:** the speed flavour's kernels are the fastest of the three in bulk copies and fills. newlib's Thumb-2 and Thumb-1 `strcmp` beats ours at 64 bytes (127–176 instructions against 240 on the Thumb-2 cores, 478 on Armv6-M).
  - **strto:** about as fast as picolibc (0.85–1.20 times our instructions), and faster than newlib-nano on the decimal conversions.
  - **malloc:** faster than picolibc, whose `realloc` moves the block. newlib-nano's nano-malloc is faster: in this single-threaded configuration it takes no lock, while rtoskit's allocator is always thread-safe through the seam (D31, D79).
- **Slower:**
  - **math:** 2–3 times the instructions of picolibc and newlib on the FPU cores, and about 2.2 times on Armv6-M. That is the cost of correct rounding (D57, D47): float-float and double-double arithmetic where the others use a single polynomial. Armv6-M's fixed-point fast paths close most of the gap for the float functions they cover.
  - **stdio:** about as fast for integers and strings.
    - `%f`/`%g` cost 3.7 times picolibc's on the FPU cores, because the exact decimal conversion (D39) handles every digit.
    - On Armv6-M, picolibc's `%f` is slower than ours, and newlib-nano's takes half our instructions.
  - **qsort:** about 2.6 times the instructions. It is a heapsort (D33), chosen for its worst case without recursion or extra memory, and no phase 3 step targeted it. Recorded as Q17.

## Addendum: P3.07, the speed flavour's qsort

After this report, Q17 was answered by an introsort with a run check in the speed flavour (D90, `docs/research/qsort-speed.md`).
- **Against phase 2:** the speed flavour's qsort geometric mean (all eight cases, bsearch included) is 0.23–0.27 on every board, from 0.98–1.00.
- **Against the other libraries:** on random input it now takes the same count as picolibc and newlib-nano (armv7e-m: 274 against 278 and 277). Over all eight cases, they take 1.35–1.49 times its instructions.

The size flavour keeps the heapsort.
