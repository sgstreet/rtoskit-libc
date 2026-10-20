# Benchmarks: armv7e-m-dp

Core: Cortex-M7 class (Thumb-2, DSP, FPv5-D16): the Thumb-1 string kernels (D70), float and double in hardware.

Method: `docs/research/benchmarks.md` (PLAN D72). Regenerate the tables with
`tools/scripts/bench-runner markdown --board linux-armv7e-m-dp [--previous <rev>]`;
`docs/perf/baseline-armv7e-m-dp.json` holds every case, including the full string sweep.

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
| `fprintf/mix` | 2341 |
| `fputs/line` | 377 |
| `snprintf/%-20s` | 614 |
| `snprintf/%.17g` | 14930 |
| `snprintf/%.3f` | 14654 |
| `snprintf/%08x` | 537 |
| `snprintf/%a` | 965 |
| `snprintf/%c` | 266 |
| `snprintf/%d` | 475 |
| `snprintf/%e` | 14035 |
| `snprintf/%f` | 14737 |
| `snprintf/%g` | 15541 |
| `snprintf/%lld` | 7643 |
| `snprintf/%p` | 457 |
| `snprintf/%s` | 423 |
| `snprintf/%u-small` | 364 |
| `snprintf/%x` | 518 |
| `snprintf/literal` | 512 |
| `snprintf/mix` | 1959 |
| `sscanf/%d` | 457 |
| `sscanf/%lf` | 2777 |
| `sscanf/%x` | 540 |
| `sscanf/mix` | 2090 |

### strto

| Case | insns/op |
|---|---:|
| `atoi` | 1081 |
| `strtod/17-digits` | 5179 |
| `strtod/exp-large` | 176132 |
| `strtod/exp-small` | 29352 |
| `strtod/fixed` | 1421 |
| `strtod/hex` | 915 |
| `strtod/integer` | 1234 |
| `strtod/long` | 8378 |
| `strtod/micro` | 1281 |
| `strtod/min-normal` | 104178 |
| `strtod/short` | 1151 |
| `strtod/subnormal` | 113845 |
| `strtof/9-digits` | 4213 |
| `strtof/exp` | 9153 |
| `strtof/short` | 1128 |
| `strtol/hex` | 1195 |
| `strtol/int` | 1213 |
| `strtol/short` | 949 |
| `strtoll/long` | 1496 |
| `strtoul/hex16` | 1179 |
| `strtoull/hex` | 1446 |

### malloc

| Case | insns/op |
|---|---:|
| `batch/churn` | 133 |
| `batch/fifo` | 112 |
| `batch/lifo` | 107 |
| `batch/shuffled` | 126 |
| `calloc/256` | 249 |
| `fixed/16` | 108 |
| `fixed/256` | 108 |
| `fixed/4096` | 108 |
| `realloc/grow` | 113 |

### malloc-tiny

| Case | insns/op |
|---|---:|
| `batch/churn` | 237 |
| `batch/fifo` | 221 |
| `batch/lifo` | 221 |
| `batch/shuffled` | 222 |
| `calloc/256` | 254 |
| `fixed/16` | 112 |
| `fixed/256` | 112 |
| `fixed/4096` | 112 |
| `realloc/grow` | 111 |

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
| `acos` | 347 | 343 |
| `acosh` | 332 | 328 |
| `asin` | 354 | 336 |
| `asinh` | 403 | 388 |
| `atan` | 283 | 267 |
| `atan2` | 470 | 432 |
| `atanh` | 284 | 267 |
| `cbrt` | 137 | 134 |
| `cos` | 224 | 220 |
| `cosh` | 219 | 216 |
| `erf` | 437 | 433 |
| `erfc` | 937 | 933 |
| `exp` | 186 | 183 |
| `exp2` | 212 | 209 |
| `expm1` | 201 | 186 |
| `floor` | 10.3 | 11.8 |
| `fmod` | 164 | 178 |
| `hypot` | 35.4 | 199 |
| `lgamma` | 619 | 615 |
| `log` | 192 | 189 |
| `log10` | 218 | 215 |
| `log1p` | 242 | 226 |
| `log2` | 254 | 251 |
| `pow` | 462 | 457 |
| `round` | 10.3 | 11.8 |
| `sin` | 234 | 218 |
| `sinh` | 247 | 230 |
| `sqrt` | 10.3 | 11.6 |
| `tan` | 324 | 307 |
| `tanh` | 264 | 248 |
| `tgamma` | 869 | 865 |

### threads

| Case | insns/op |
|---|---:|
| `call_once/done` | 19.0 |
| `mtx/nested` | 147 |
| `mtx/plain` | 84.0 |
| `mtx/recursive` | 87.0 |
| `mtx/timed` | 84.0 |
| `mtx/trylock` | 78.0 |
| `tss_get` | 34.0 |

