# Benchmarks: armv6s-m

Core: Armv6-M with SVC (same code as armv6-m).

Method: `docs/research/benchmarks.md` (PLAN D72). Regenerate the tables with
`tools/scripts/bench-runner [--build-type speed] markdown --board linux-armv6s-m [--previous <rev> | --against <file>]`;
`docs/perf/baseline-armv6s-m.json` (size flavour) and `docs/perf/baseline-armv6s-m-speed.json` (speed flavour, PLAN D73)
hold every case, including the full string sweep.

## P3.01 — Phase 2 baseline

The library as Phase 2 left it (`phase-2-complete`), size flavour, release.

Instructions per operation (qemu 8.2.2, release, size flavour).

### string

Aligned / worst of the misaligned offsets.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memcpy` | 34.0 / 29.0 | 38.0 / 38.0 | 39.0 / 65.0 | 54.0 / 93.0 | 114 / 285 | 354 / 1053 | 1314 / 4125 |
| `memmove-down` | 40.0 / 35.0 | 44.0 / 44.0 | 45.0 / 71.0 | 60.0 / 99.0 | 120 / 291 | 360 / 1059 | 1320 / 4131 |
| `memmove-up` | 40.0 / 35.0 | 44.0 / 44.0 | 45.0 / 71.0 | 53.0 / 107 | 113 / 347 | 353 / 1307 | 1313 / 5147 |
| `memset` | 30.0 / 28.0 | 33.0 / 37.0 | 34.0 / 54.0 | 46.0 / 66.0 | 94.0 / 114 | 286 / 306 | 1054 / 1074 |
| `memcmp` | 28.0 / 28.0 | 35.0 / 35.0 | 56.0 / 56.0 | 140 / 140 | 476 / 476 | 1820 / 1820 | 7196 / 7196 |
| `memchr` | 29.0 / 29.0 | 35.0 / 29.0 | 64.0 / 65.0 | 97.0 / 102 | 229 / 234 | 757 / 762 | 2869 / 2874 |
| `strlen` | 35.0 / 28.0 | 42.0 / 42.0 | 40.0 / 63.0 | 55.0 / 78.0 | 115 / 138 | 355 / 378 | 1315 / 1338 |
| `strchr` | 54.0 / 39.0 | 52.0 / 37.0 | 70.0 / 79.0 | 109 / 118 | 265 / 274 | 889 / 898 | 3385 / 3394 |
| `strcmp` | 29.0 / 29.0 | 37.0 / 37.0 | 61.0 / 61.0 | 157 / 157 | 541 / 541 | 2077 / 2077 | 8221 / 8221 |
| `strcpy` | 66.0 / 61.0 | 77.0 / 75.0 | 76.0 / 125 | 106 / 168 | 226 / 420 | 706 / 1428 | 2626 / 5460 |
| `mix/memcpy` (per call) | 223 | | | | | | |
| `mix/memset` (per call) | 95.6 | | | | | | |
| `mix/strlen` (per call) | 120 | | | | | | |

### stdio

| Case | insns/op |
|---|---:|
| `fprintf/mix` | 4143 |
| `fputs/line` | 503 |
| `snprintf/%-20s` | 1166 |
| `snprintf/%.17g` | 13988 |
| `snprintf/%.3f` | 13867 |
| `snprintf/%08x` | 918 |
| `snprintf/%a` | 1204 |
| `snprintf/%c` | 292 |
| `snprintf/%d` | 1638 |
| `snprintf/%e` | 16998 |
| `snprintf/%f` | 13287 |
| `snprintf/%g` | 14849 |
| `snprintf/%lld` | 11027 |
| `snprintf/%p` | 539 |
| `snprintf/%s` | 451 |
| `snprintf/%u-small` | 431 |
| `snprintf/%x` | 631 |
| `snprintf/literal` | 555 |
| `snprintf/mix` | 3564 |
| `sscanf/%d` | 784 |
| `sscanf/%lf` | 5107 |
| `sscanf/%x` | 966 |
| `sscanf/mix` | 3311 |

### strto

| Case | insns/op |
|---|---:|
| `atoi` | 1211 |
| `strtod/17-digits` | 24062 |
| `strtod/exp-large` | 692937 |
| `strtod/exp-small` | 416299 |
| `strtod/fixed` | 3863 |
| `strtod/hex` | 2101 |
| `strtod/integer` | 2909 |
| `strtod/long` | 47601 |
| `strtod/micro` | 3368 |
| `strtod/min-normal` | 1850827 |
| `strtod/short` | 3352 |
| `strtod/subnormal` | 2040498 |
| `strtof/9-digits` | 16890 |
| `strtof/exp` | 78890 |
| `strtof/short` | 1634 |
| `strtol/hex` | 1678 |
| `strtol/int` | 1501 |
| `strtol/short` | 914 |
| `strtoll/long` | 2350 |
| `strtoul/hex16` | 1671 |
| `strtoull/hex` | 2544 |

### malloc

| Case | insns/op |
|---|---:|
| `batch/churn` | 265 |
| `batch/fifo` | 237 |
| `batch/lifo` | 234 |
| `batch/shuffled` | 256 |
| `calloc/256` | 378 |
| `fixed/16` | 233 |
| `fixed/256` | 233 |
| `fixed/4096` | 233 |
| `realloc/grow` | 233 |

### malloc-tiny

| Case | insns/op |
|---|---:|
| `batch/churn` | 389 |
| `batch/fifo` | 368 |
| `batch/lifo` | 368 |
| `batch/shuffled` | 369 |
| `calloc/256` | 386 |
| `fixed/16` | 240 |
| `fixed/256` | 240 |
| `fixed/4096` | 240 |
| `realloc/grow` | 232 |

### qsort

| Case | insns/op |
|---|---:|
| `bsearch/1024` | 167 |
| `qsort/equal/1024` | 139 |
| `qsort/random/1024` | 769 |
| `qsort/random/16` | 267 |
| `qsort/random/256` | 596 |
| `qsort/reversed/1024` | 723 |
| `qsort/sorted/1024` | 813 |
| `qsort/strings/256` | 707 |

### math

Throughput, per call (the latency counts are the same under qemu).

| Function | float | double |
|---|---:|---:|
| `acos` | 26692 | 35771 |
| `acosh` | 22250 | 34288 |
| `asin` | 25875 | 34679 |
| `asinh` | 21693 | 39824 |
| `atan` | 18770 | 21229 |
| `atan2` | 19460 | 22656 |
| `atanh` | 16542 | 25888 |
| `cbrt` | 9412 | 14545 |
| `cos` | 18928 | 17700 |
| `cosh` | 15893 | 16736 |
| `erf` | 151747 | 40695 |
| `erfc` | 128215 | 112996 |
| `exp` | 11458 | 10356 |
| `exp2` | 9609 | 14172 |
| `expm1` | 12742 | 10978 |
| `floor` | 105 | 212 |
| `fmod` | 273 | 486 |
| `hypot` | 8371 | 14812 |
| `lgamma` | 38415 | 47161 |
| `log` | 12864 | 17003 |
| `log10` | 12862 | 21106 |
| `log1p` | 13273 | 18064 |
| `log2` | 10973 | 21459 |
| `pow` | 23058 | 31562 |
| `round` | 103 | 210 |
| `sin` | 18996 | 17710 |
| `sinh` | 16022 | 17881 |
| `sqrt` | 2036 | 4208 |
| `tan` | 37649 | 32504 |
| `tanh` | 16684 | 20206 |
| `tgamma` | 57787 | 64810 |

### threads

| Case | insns/op |
|---|---:|
| `call_once/done` | 22.0 |
| `mtx/nested` | 275 |
| `mtx/plain` | 205 |
| `mtx/recursive` | 208 |
| `mtx/timed` | 205 |
| `mtx/trylock` | 203 |
| `tss_get` | 37.0 |

## P3.02a — Speed flavour: Thumb-2 memory kernels

The speed flavour (`BUILD_TYPE=speed`, PLAN D73) had no speed kernels here yet: its figures equal the size
flavour's.

Instructions per operation (qemu 8.2.2, speed, speed flavour), change against docs/perf/baseline-armv6s-m.json.

### string

Aligned / worst of the misaligned offsets.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memcpy` | 34.0 / 29.0 | 38.0 / 38.0 | 39.0 / 65.0 | 54.0 / 93.0 | 114 / 285 | 354 / 1053 | 1314 / 4125 |
| `memmove-down` | 40.0 / 35.0 | 44.0 / 44.0 | 45.0 / 71.0 | 60.0 / 99.0 | 120 / 291 | 360 / 1059 | 1320 / 4131 |
| `memmove-up` | 40.0 / 35.0 | 44.0 / 44.0 | 45.0 / 71.0 | 53.0 / 107 | 113 / 347 | 353 / 1307 | 1313 / 5147 |
| `memset` | 30.0 / 28.0 | 33.0 / 37.0 | 34.0 / 54.0 | 46.0 / 66.0 | 94.0 / 114 | 286 / 306 | 1054 / 1074 |
| `memcmp` | 28.0 / 28.0 | 35.0 / 35.0 | 56.0 / 56.0 | 140 / 140 | 476 / 476 | 1820 / 1820 | 7196 / 7196 |
| `memchr` | 29.0 / 29.0 | 35.0 / 29.0 | 64.0 / 65.0 | 97.0 / 102 | 229 / 234 | 757 / 762 | 2869 / 2874 |
| `strlen` | 35.0 / 28.0 | 42.0 / 42.0 | 40.0 / 63.0 | 55.0 / 78.0 | 115 / 138 | 355 / 378 | 1315 / 1338 |
| `strchr` | 54.0 / 39.0 | 52.0 / 37.0 | 70.0 / 79.0 | 109 / 118 | 265 / 274 | 889 / 898 | 3385 / 3394 |
| `strcmp` | 29.0 / 29.0 | 37.0 / 37.0 | 61.0 / 61.0 | 157 / 157 | 541 / 541 | 2077 / 2077 | 8221 / 8221 |
| `strcpy` | 66.0 / 61.0 | 77.0 / 75.0 | 76.0 / 125 | 106 / 168 | 226 / 420 | 706 / 1428 | 2626 / 5460 |
| `mix/memcpy` (per call) | 223 | | | | | | |
| `mix/memset` (per call) | 95.6 | | | | | | |
| `mix/strlen` (per call) | 120 | | | | | | |

### stdio

| Case | insns/op |
|---|---:|
| `fprintf/mix` | 4143 |
| `fputs/line` | 503 |
| `snprintf/%-20s` | 1166 |
| `snprintf/%.17g` | 13988 |
| `snprintf/%.3f` | 13867 |
| `snprintf/%08x` | 918 |
| `snprintf/%a` | 1204 |
| `snprintf/%c` | 292 |
| `snprintf/%d` | 1638 |
| `snprintf/%e` | 16998 |
| `snprintf/%f` | 13287 |
| `snprintf/%g` | 14849 |
| `snprintf/%lld` | 11027 |
| `snprintf/%p` | 539 |
| `snprintf/%s` | 451 |
| `snprintf/%u-small` | 431 |
| `snprintf/%x` | 631 |
| `snprintf/literal` | 555 |
| `snprintf/mix` | 3564 |
| `sscanf/%d` | 784 |
| `sscanf/%lf` | 5107 |
| `sscanf/%x` | 966 |
| `sscanf/mix` | 3311 |

### strto

| Case | insns/op |
|---|---:|
| `atoi` | 1211 |
| `strtod/17-digits` | 24062 |
| `strtod/exp-large` | 692937 |
| `strtod/exp-small` | 416299 |
| `strtod/fixed` | 3863 |
| `strtod/hex` | 2101 |
| `strtod/integer` | 2909 |
| `strtod/long` | 47601 |
| `strtod/micro` | 3368 |
| `strtod/min-normal` | 1850827 |
| `strtod/short` | 3352 |
| `strtod/subnormal` | 2040498 |
| `strtof/9-digits` | 16890 |
| `strtof/exp` | 78890 |
| `strtof/short` | 1634 |
| `strtol/hex` | 1678 |
| `strtol/int` | 1501 |
| `strtol/short` | 914 |
| `strtoll/long` | 2350 |
| `strtoul/hex16` | 1671 |
| `strtoull/hex` | 2544 |

### malloc

| Case | insns/op |
|---|---:|
| `batch/churn` | 265 |
| `batch/fifo` | 237 |
| `batch/lifo` | 234 |
| `batch/shuffled` | 256 |
| `calloc/256` | 378 |
| `fixed/16` | 233 |
| `fixed/256` | 233 |
| `fixed/4096` | 233 |
| `realloc/grow` | 233 |

### malloc-tiny

| Case | insns/op |
|---|---:|
| `batch/churn` | 389 |
| `batch/fifo` | 368 |
| `batch/lifo` | 368 |
| `batch/shuffled` | 369 |
| `calloc/256` | 386 |
| `fixed/16` | 240 |
| `fixed/256` | 240 |
| `fixed/4096` | 240 |
| `realloc/grow` | 232 |

### qsort

| Case | insns/op |
|---|---:|
| `bsearch/1024` | 167 |
| `qsort/equal/1024` | 139 |
| `qsort/random/1024` | 769 |
| `qsort/random/16` | 267 |
| `qsort/random/256` | 596 |
| `qsort/reversed/1024` | 723 |
| `qsort/sorted/1024` | 813 |
| `qsort/strings/256` | 707 |

### math

Throughput, per call (the latency counts are the same under qemu).

| Function | float | double |
|---|---:|---:|
| `acos` | 26692 | 35771 |
| `acosh` | 22250 | 34288 |
| `asin` | 25875 | 34679 |
| `asinh` | 21693 | 39824 |
| `atan` | 18770 | 21229 |
| `atan2` | 19460 | 22656 |
| `atanh` | 16542 | 25888 |
| `cbrt` | 9412 | 14545 |
| `cos` | 18928 | 17700 |
| `cosh` | 15893 | 16736 |
| `erf` | 151747 | 40695 |
| `erfc` | 128215 | 112996 |
| `exp` | 11458 | 10356 |
| `exp2` | 9609 | 14172 |
| `expm1` | 12742 | 10978 |
| `floor` | 105 | 212 |
| `fmod` | 273 | 486 |
| `hypot` | 8371 | 14812 |
| `lgamma` | 38415 | 47161 |
| `log` | 12864 | 17003 |
| `log10` | 12862 | 21106 |
| `log1p` | 13273 | 18064 |
| `log2` | 10973 | 21459 |
| `pow` | 23058 | 31562 |
| `round` | 103 | 210 |
| `sin` | 18996 | 17710 |
| `sinh` | 16022 | 17881 |
| `sqrt` | 2036 | 4208 |
| `tan` | 37649 | 32504 |
| `tanh` | 16684 | 20206 |
| `tgamma` | 57787 | 64810 |

### threads

| Case | insns/op |
|---|---:|
| `call_once/done` | 22.0 |
| `mtx/nested` | 275 |
| `mtx/plain` | 205 |
| `mtx/recursive` | 208 |
| `mtx/timed` | 205 |
| `mtx/trylock` | 203 |
| `tss_get` | 37.0 |

## P3.02c — Speed flavour: Armv6-M kernels

The Armv6-M speed kernels `memcpy`/`memmove`/`memset`/`strcmp` in `libc/string/thumb1-speed` (PLAN D75).
Change against the previous speed baseline (equal to the phase 2 size baseline here).

### string

Aligned / worst of the misaligned offsets.

| Function | 0 | 1 | 4 | 16 | 64 | 256 | 1024 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `memcpy` | 36.0 (+6 %) / 30.0 | 40.0 (+5 %) / 37.0 | 40.0 (+3 %) / 67.0 | 40.0 (-26 %) / 94.0 | 52.0 (-54 %) / 187 | 100 (-72 %) / 523 | 292 (-78 %) / 1867 |
| `memmove-down` | 42.0 (+5 %) / 36.0 | 46.0 (+5 %) / 43.0 | 46.0 (+2 %) / 73.0 | 46.0 (-23 %) / 100 | 58.0 (-52 %) / 193 | 106 (-71 %) / 529 | 298 (-77 %) / 1873 |
| `memmove-up` | 42.0 (+5 %) / 36.0 | 46.0 (+5 %) / 43.0 | 46.0 (+2 %) / 73.0 | 42.0 (-21 %) / 94.0 | 60.0 (-47 %) / 214 | 132 (-63 %) / 646 | 420 (-68 %) / 2374 |
| `memset` | 32.0 (+7 %) / 28.0 | 35.0 (+6 %) / 39.0 | 35.0 (+3 %) / 56.0 | 44.0 (-4 %) / 65.0 | 49.0 (-48 %) / 80.0 | 73.0 (-74 %) / 104 | 169 (-84 %) / 200 |
| `memcmp` | 28.0 / 28.0 | 35.0 / 35.0 | 56.0 / 56.0 | 140 / 140 | 476 / 476 | 1820 / 1820 | 7196 / 7196 |
| `memchr` | 29.0 / 29.0 | 35.0 / 29.0 | 64.0 / 65.0 | 97.0 / 102 | 229 / 234 | 757 / 762 | 2869 / 2874 |
| `strlen` | 35.0 / 28.0 | 42.0 / 42.0 | 40.0 / 63.0 | 55.0 / 78.0 | 115 / 138 | 355 / 378 | 1315 / 1338 |
| `strchr` | 54.0 / 39.0 | 52.0 / 37.0 | 70.0 / 79.0 | 109 / 118 | 265 / 274 | 889 / 898 | 3385 / 3394 |
| `strcmp` | 30.0 (+3 %) / 30.0 | 37.0 / 37.0 | 58.0 (-5 %) / 58.0 | 142 (-10 %) / 142 | 478 (-12 %) / 478 | 1822 (-12 %) / 1822 | 7198 (-12 %) / 7198 |
| `strcpy` | 68.0 (+3 %) / 62.0 | 79.0 (+3 %) / 74.0 | 77.0 (+1 %) / 127 | 92.0 (-13 %) / 169 | 164 (-27 %) / 318 | 452 (-36 %) / 894 | 1604 (-39 %) / 3198 |
| `mix/memcpy` (per call) | 136 (-39 %) | | | | | | |
| `mix/memset` (per call) | 60.4 (-37 %) | | | | | | |
| `mix/strlen` (per call) | 120 | | | | | | |

### stdio

| Case | insns/op |
|---|---:|
| `fprintf/mix` | 4142 (-0 %) |
| `fputs/line` | 451 (-10 %) |
| `snprintf/%-20s` | 1164 (-0 %) |
| `snprintf/%.17g` | 13910 (-1 %) |
| `snprintf/%.3f` | 13789 (-1 %) |
| `snprintf/%08x` | 916 (-0 %) |
| `snprintf/%a` | 1126 (-6 %) |
| `snprintf/%c` | 290 (-1 %) |
| `snprintf/%d` | 1636 (-0 %) |
| `snprintf/%e` | 16913 (-1 %) |
| `snprintf/%f` | 13209 (-1 %) |
| `snprintf/%g` | 14771 (-1 %) |
| `snprintf/%lld` | 11025 (-0 %) |
| `snprintf/%p` | 537 (-0 %) |
| `snprintf/%s` | 449 (-0 %) |
| `snprintf/%u-small` | 429 (-0 %) |
| `snprintf/%x` | 629 (-0 %) |
| `snprintf/literal` | 555 |
| `snprintf/mix` | 3556 (-0 %) |
| `sscanf/%d` | 784 |
| `sscanf/%lf` | 5107 |
| `sscanf/%x` | 966 |
| `sscanf/mix` | 3294 (-1 %) |

### strto

| Case | insns/op |
|---|---:|
| `atoi` | 1211 |
| `strtod/17-digits` | 23288 (-3 %) |
| `strtod/exp-large` | 689247 (-1 %) |
| `strtod/exp-small` | 413319 (-1 %) |
| `strtod/fixed` | 3863 |
| `strtod/hex` | 2101 |
| `strtod/integer` | 2909 |
| `strtod/long` | 46683 (-2 %) |
| `strtod/micro` | 3368 |
| `strtod/min-normal` | 1839969 (-1 %) |
| `strtod/short` | 3352 |
| `strtod/subnormal` | 2028570 (-1 %) |
| `strtof/9-digits` | 16152 (-4 %) |
| `strtof/exp` | 77753 (-1 %) |
| `strtof/short` | 1634 |
| `strtol/hex` | 1678 |
| `strtol/int` | 1501 |
| `strtol/short` | 914 |
| `strtoll/long` | 2350 |
| `strtoul/hex16` | 1671 |
| `strtoull/hex` | 2544 |

### malloc

| Case | insns/op |
|---|---:|
| `batch/churn` | 265 |
| `batch/fifo` | 237 |
| `batch/lifo` | 234 |
| `batch/shuffled` | 256 |
| `calloc/256` | 272 (-28 %) |
| `fixed/16` | 233 |
| `fixed/256` | 233 |
| `fixed/4096` | 233 |
| `realloc/grow` | 233 |

### malloc-tiny

| Case | insns/op |
|---|---:|
| `batch/churn` | 389 |
| `batch/fifo` | 368 |
| `batch/lifo` | 368 |
| `batch/shuffled` | 369 |
| `calloc/256` | 279 (-28 %) |
| `fixed/16` | 240 |
| `fixed/256` | 240 |
| `fixed/4096` | 240 |
| `realloc/grow` | 232 |

### qsort

| Case | insns/op |
|---|---:|
| `bsearch/1024` | 167 |
| `qsort/equal/1024` | 139 |
| `qsort/random/1024` | 769 |
| `qsort/random/16` | 267 |
| `qsort/random/256` | 596 |
| `qsort/reversed/1024` | 723 |
| `qsort/sorted/1024` | 813 |
| `qsort/strings/256` | 708 (+0 %) |

### math

Throughput, per call (the latency counts are the same under qemu).

| Function | float | double |
|---|---:|---:|
| `acos` | 26692 | 35757 (-0 %) |
| `acosh` | 22250 | 34288 |
| `asin` | 25875 | 34665 (-0 %) |
| `asinh` | 21693 | 39824 |
| `atan` | 18770 | 21215 (-0 %) |
| `atan2` | 19460 | 22642 (-0 %) |
| `atanh` | 16542 | 25874 (-0 %) |
| `cbrt` | 9412 | 14545 |
| `cos` | 18928 | 17700 |
| `cosh` | 15893 | 16736 |
| `erf` | 151747 | 40695 |
| `erfc` | 128215 | 112996 |
| `exp` | 11458 | 10356 |
| `exp2` | 9609 | 14172 |
| `expm1` | 12742 | 10978 |
| `floor` | 105 | 212 |
| `fmod` | 273 | 486 |
| `hypot` | 8371 | 14812 |
| `lgamma` | 38415 | 47123 (-0 %) |
| `log` | 12864 | 17003 |
| `log10` | 12862 | 21106 |
| `log1p` | 13273 | 18064 |
| `log2` | 10973 | 21459 |
| `pow` | 23058 | 31562 |
| `round` | 103 | 210 |
| `sin` | 18996 | 17710 |
| `sinh` | 16022 | 17881 |
| `sqrt` | 2036 | 4208 |
| `tan` | 37649 | 32490 (-0 %) |
| `tanh` | 16684 | 20192 (-0 %) |
| `tgamma` | 57787 | 64762 (-0 %) |

### threads

| Case | insns/op |
|---|---:|
| `call_once/done` | 22.0 |
| `mtx/nested` | 275 |
| `mtx/plain` | 205 |
| `mtx/recursive` | 208 |
| `mtx/timed` | 205 |
| `mtx/trylock` | 203 |
| `tss_get` | 37.0 |
