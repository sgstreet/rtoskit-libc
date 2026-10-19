# Size: armv7e-m-dp

Release build of `linux-armv7e-m-dp`, measured with `tools/scripts/size-report` (PLAN D51,
`docs/research/size-measurement.md`). The figures are in `baseline-armv7e-m-dp.json`;
`tools/scripts/size-report diff` compares a build with them. Each phase 2 step
records its reductions, and justifies any growth, in a section below.

## P2.01 baseline (end of phase 1)

`libc.a` (743 members), release build:

| text | rodata | unwind | data | bss | tdata | tbss |
|---:|---:|---:|---:|---:|---:|---:|
| 77666 | 8879 | 0 | 116 | 672 | 8 | 396 |

Reference images (`test/size`): code is text+rodata+unwind, RAM is data+bss, TLS is tdata+tbss per thread; the libc and port shares come from the link map:

| Program | code | RAM | TLS | libc code | libc RAM | libc TLS | port code | libgcc code | main stack |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| hello-puts | 2595 | 356 | 8 | 1738 | 224 | 4 | 818 | 0 | 136* |
| hello-printf-int | 8612 | 356 | 8 | 6963 | 224 | 4 | 818 | 756 | 1268+ |
| hello-printf-float | 8636 | 356 | 8 | 6963 | 224 | 4 | 818 | 756 | 1276+ |
| malloc-heavy | 2024 | 228 | 8 | 1106 | 144 | 4 | 716 | 0 | 184* |
| math-mix | 13708 | 200 | 4 | 11706 | 132 | 0 | 656 | 888 | 672* |

Worst stack depth in bytes. `*`: without the frames of assembly or libgcc helpers (leaf routines that push a few registers); `+`: a lower bound because of an indirect call, recursion, a dynamic frame or an unknown callee:

| Function | depth | Function | depth | Function | depth |
|---|---:|---|---:|---|---:|
| `printf` | 1252+ | `fprintf` | 1248+ | `snprintf` | 1244+ |
| `sscanf` | 3168+ | `fscanf` | 3176+ | `strtod` | 1084+ |
| `strtof` | 1084+ | `strtol` | 88* | `malloc` | 72* |
| `calloc` | 88* | `realloc` | 88* | `free` | 72* |
| `qsort` | 68+ | `bsearch` | 32+ | `strftime` | 400+ |
| `mktime` | 400* | `localtime` | 292* | `gmtime` | 44* |
| `fopen` | 120* | `fclose` | 88* | `fputs` | 128* |
| `puts` | 128* | `fgets` | 120* | `fflush` | 112* |
| `exit` | 88+ | `abort` | 48+ | `thrd_create` | 104* |
| `mtx_lock` | 72* | `cnd_wait` | 96* | `exp` | 144 |
| `log` | 168 | `pow` | 248 | `sin` | 240* |
| `cos` | 240* | `tan` | 288* | `atan2` | 560 |
| `lgamma` | 896 | `tgamma` | 896 | `erfc` | 304 |
| `cpow` | 1296* | `sinf` | 280* | `expf` | 152 |
| `powf` | 256 |  |  |  |  |

Largest members (text+rodata+unwind+data+bss+tdata+tbss), top 15:

| Member | text | rodata | unwind | data | bss | tdata | tbss |
|---|---:|---:|---:|---:|---:|---:|---:|
| `strerror.o` | 24 | 2371 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format.o` | 2200 | 96 | 0 | 0 | 0 | 0 | 0 |
| `strftime.o` | 1918 | 285 | 0 | 0 | 0 | 0 | 0 |
| `__libc_erfc_table.o` | 348 | 1824 | 0 | 0 | 0 | 0 | 0 |
| `__libc_log_core.o` | 652 | 1472 | 0 | 0 | 0 | 0 | 0 |
| `__libc_lgamma_core.o` | 1610 | 392 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format_float.o` | 1854 | 90 | 0 | 0 | 0 | 0 | 0 |
| `__libc_scan.o` | 1728 | 0 | 0 | 0 | 0 | 0 | 0 |
| `cpow.o` | 1474 | 16 | 0 | 0 | 0 | 0 | 0 |
| `__libc_exp_core.o` | 396 | 1024 | 0 | 0 | 0 | 0 | 0 |
| `__libc_atan2_core.o` | 752 | 528 | 0 | 0 | 0 | 0 | 0 |
| `__libc_strtofloat.o` | 1114 | 17 | 0 | 0 | 0 | 0 | 0 |
| `__libc_scan_float.o` | 1080 | 25 | 0 | 0 | 0 | 0 | 0 |
| `__libc_rem_pio2.o` | 934 | 160 | 0 | 0 | 0 | 0 | 0 |
| `__libc_exact_fma.o` | 992 | 0 | 0 | 0 | 0 | 0 | 0 |
