# Benchmarks: armv8-m.main

Core: Cortex-M33 class (Thumb-2, DSP, FPv5-SP): the Thumb-1 string kernels (D70), float in hardware and float-float, double in software.

Method: `docs/research/benchmarks.md` (PLAN D72). Regenerate the tables with
`tools/scripts/bench-runner markdown --board linux-armv8-m.main [--previous <rev>]`;
`docs/perf/baseline-armv8-m.main.json` holds every case, including the full string sweep.

## P3.01 — Phase 2 baseline

The library as Phase 2 left it (`phase-2-complete`), size flavour, release.

Instructions per operation (qemu 8.2.2, release, size flavour).

### string

Aligned / worst of the misaligned offsets.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memcpy` | 33.0 / 28.0 | 37.0 / 37.0 | 38.0 / 64.0 | 53.0 / 92.0 | 113 / 284 | 353 / 1052 | 1313 / 4124 |
| `memmove-down` | 39.0 / 34.0 | 43.0 / 43.0 | 44.0 / 70.0 | 59.0 / 98.0 | 119 / 290 | 359 / 1058 | 1319 / 4130 |
| `memmove-up` | 39.0 / 34.0 | 43.0 / 43.0 | 44.0 / 70.0 | 52.0 / 106 | 112 / 346 | 352 / 1306 | 1312 / 5146 |
| `memset` | 30.0 / 28.0 | 33.0 / 37.0 | 34.0 / 54.0 | 46.0 / 66.0 | 94.0 / 114 | 286 / 306 | 1054 / 1074 |
| `memcmp` | 27.0 / 27.0 | 35.0 / 35.0 | 59.0 / 59.0 | 155 / 155 | 539 / 539 | 2075 / 2075 | 8219 / 8219 |
| `memchr` | 29.0 / 29.0 | 36.0 / 28.0 | 65.0 / 66.0 | 95.0 / 98.0 | 215 / 218 | 695 / 698 | 2615 / 2618 |
| `strlen` | 35.0 / 28.0 | 42.0 / 42.0 | 40.0 / 63.0 | 55.0 / 78.0 | 115 / 138 | 355 / 378 | 1315 / 1338 |
| `strchr` | 52.0 / 38.0 | 48.0 / 35.0 | 69.0 / 75.0 | 102 / 108 | 234 / 240 | 762 / 768 | 2874 / 2880 |
| `strcmp` | 28.0 / 28.0 | 36.0 / 36.0 | 60.0 / 60.0 | 156 / 156 | 540 / 540 | 2076 / 2076 | 8220 / 8220 |
| `strcpy` | 65.0 / 60.0 | 76.0 / 74.0 | 75.0 / 124 | 105 / 167 | 225 / 419 | 705 / 1427 | 2625 / 5459 |
| `mix/memcpy` (per call) | 215 | | | | | | |
| `mix/memset` (per call) | 89.6 | | | | | | |
| `mix/strlen` (per call) | 113 | | | | | | |

### stdio

| Case | insns/op |
|---|---:|
| `fprintf/mix` | 2338 |
| `fputs/line` | 375 |
| `snprintf/%-20s` | 614 |
| `snprintf/%.17g` | 14926 |
| `snprintf/%.3f` | 15224 |
| `snprintf/%08x` | 537 |
| `snprintf/%a` | 961 |
| `snprintf/%c` | 266 |
| `snprintf/%d` | 475 |
| `snprintf/%e` | 14031 |
| `snprintf/%f` | 14733 |
| `snprintf/%g` | 15537 |
| `snprintf/%lld` | 7643 |
| `snprintf/%p` | 457 |
| `snprintf/%s` | 423 |
| `snprintf/%u-small` | 364 |
| `snprintf/%x` | 518 |
| `snprintf/literal` | 512 |
| `snprintf/mix` | 1959 |
| `sscanf/%d` | 457 |
| `sscanf/%lf` | 3403 |
| `sscanf/%x` | 540 |
| `sscanf/mix` | 2089 |

### strto

| Case | insns/op |
|---|---:|
| `atoi` | 1081 |
| `strtod/17-digits` | 5396 |
| `strtod/exp-large` | 176378 |
| `strtod/exp-small` | 29563 |
| `strtod/fixed` | 2232 |
| `strtod/hex` | 1112 |
| `strtod/integer` | 1497 |
| `strtod/long` | 8595 |
| `strtod/micro` | 2086 |
| `strtod/min-normal` | 104395 |
| `strtod/short` | 1962 |
| `strtod/subnormal` | 114674 |
| `strtof/9-digits` | 4230 |
| `strtof/exp` | 9170 |
| `strtof/short` | 1125 |
| `strtol/hex` | 1195 |
| `strtol/int` | 1213 |
| `strtol/short` | 949 |
| `strtoll/long` | 1496 |
| `strtoul/hex16` | 1179 |
| `strtoull/hex` | 1446 |

### malloc

| Case | insns/op |
|---|---:|
| `batch/churn` | 131 |
| `batch/fifo` | 110 |
| `batch/lifo` | 105 |
| `batch/shuffled` | 124 |
| `calloc/256` | 247 |
| `fixed/16` | 106 |
| `fixed/256` | 106 |
| `fixed/4096` | 106 |
| `realloc/grow` | 111 |

### malloc-tiny

| Case | insns/op |
|---|---:|
| `batch/churn` | 235 |
| `batch/fifo` | 219 |
| `batch/lifo` | 219 |
| `batch/shuffled` | 220 |
| `calloc/256` | 252 |
| `fixed/16` | 110 |
| `fixed/256` | 110 |
| `fixed/4096` | 110 |
| `realloc/grow` | 109 |

### qsort

| Case | insns/op |
|---|---:|
| `bsearch/1024` | 143 |
| `qsort/equal/1024` | 114 |
| `qsort/random/1024` | 640 |
| `qsort/random/16` | 224 |
| `qsort/random/256` | 497 |
| `qsort/reversed/1024` | 602 |
| `qsort/sorted/1024` | 677 |
| `qsort/strings/256` | 595 |

### math

Throughput, per call (the latency counts are the same under qemu).

| Function | float | double |
|---|---:|---:|
| `acos` | 427 | 16391 |
| `acosh` | 387 | 15690 |
| `asin` | 418 | 15803 |
| `asinh` | 412 | 18322 |
| `atan` | 344 | 9740 |
| `atan2` | 470 | 10501 |
| `atanh` | 301 | 11882 |
| `cbrt` | 191 | 6937 |
| `cos` | 684 | 6825 |
| `cosh` | 342 | 7119 |
| `erf` | 2538 | 16961 |
| `erfc` | 1854 | 51737 |
| `exp` | 227 | 4185 |
| `exp2` | 198 | 5776 |
| `expm1` | 260 | 4481 |
| `floor` | 10.3 | 119 |
| `fmod` | 163 | 281 |
| `hypot` | 216 | 6520 |
| `lgamma` | 802 | 21092 |
| `log` | 233 | 7530 |
| `log10` | 233 | 9093 |
| `log1p` | 265 | 8079 |
| `log2` | 212 | 9231 |
| `pow` | 480 | 13391 |
| `round` | 10.3 | 117 |
| `sin` | 692 | 6876 |
| `sinh` | 370 | 7566 |
| `sqrt` | 10.3 | 2009 |
| `tan` | 1029 | 12927 |
| `tanh` | 332 | 8538 |
| `tgamma` | 1194 | 28717 |

### threads

| Case | insns/op |
|---|---:|
| `call_once/done` | 18.0 |
| `mtx/nested` | 145 |
| `mtx/plain` | 82.0 |
| `mtx/recursive` | 85.0 |
| `mtx/timed` | 82.0 |
| `mtx/trylock` | 76.0 |
| `tss_get` | 34.0 |

