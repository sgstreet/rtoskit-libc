# Size: armv6-m

Release build of `linux-armv6-m`, measured with `tools/scripts/size-report` (PLAN D51,
`docs/research/size-measurement.md`). The figures are in `baseline-armv6-m.json`;
`tools/scripts/size-report diff` compares a build with them. Each phase 2 step
records its reductions, and justifies any growth, in a section below.

## P2.01 baseline (end of phase 1)

`libc.a` (743 members), release build:

| text | rodata | unwind | data | bss | tdata | tbss |
|---:|---:|---:|---:|---:|---:|---:|
| 112594 | 8815 | 0 | 116 | 672 | 8 | 396 |

Reference images (`test/size`): code is text+rodata+unwind, RAM is data+bss, TLS is tdata+tbss per thread; the libc and port shares come from the link map:

| Program | code | RAM | TLS | libc code | libc RAM | libc TLS | port code | libgcc code | main stack |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| hello-puts | 2887 | 356 | 8 | 1996 | 224 | 4 | 852 | 0 | 176+ |
| hello-printf-int | 8829 | 356 | 8 | 7105 | 224 | 4 | 852 | 800 | 1336+ |
| hello-printf-float | 11374 | 356 | 8 | 7105 | 224 | 4 | 852 | 3304 | 1360+ |
| malloc-heavy | 2332 | 228 | 8 | 1390 | 144 | 4 | 752 | 0 | 232+ |
| math-mix | 30720 | 200 | 4 | 20198 | 132 | 0 | 688 | 9368 | 560* |

Worst stack depth in bytes. `*`: without the frames of assembly or libgcc helpers (leaf routines that push a few registers); `+`: a lower bound because of an indirect call, recursion, a dynamic frame or an unknown callee:

| Function | depth | Function | depth | Function | depth |
|---|---:|---|---:|---|---:|
| `printf` | 1320+ | `fprintf` | 1316+ | `snprintf` | 1304+ |
| `sscanf` | 3228+ | `fscanf` | 3236+ | `strtod` | 1112+ |
| `strtof` | 1112+ | `strtol` | 88* | `malloc` | 104+ |
| `calloc` | 120+ | `realloc` | 128+ | `free` | 104+ |
| `qsort` | 84+ | `bsearch` | 32+ | `strftime` | 424+ |
| `mktime` | 384* | `localtime` | 272* | `gmtime` | 80* |
| `fopen` | 160+ | `fclose` | 120+ | `fputs` | 168+ |
| `puts` | 168+ | `fgets` | 168+ | `fflush` | 144+ |
| `exit` | 120+ | `abort` | 56+ | `thrd_create` | 184+ |
| `mtx_lock` | 96+ | `cnd_wait` | 128+ | `exp` | 200* |
| `log` | 248* | `pow` | 368* | `sin` | 320* |
| `cos` | 320* | `tan` | 384* | `atan2` | 448* |
| `lgamma` | 816* | `tgamma` | 784* | `erfc` | 392* |
| `cpow` | 1168* | `sinf` | 360* | `expf` | 208* |
| `powf` | 384* |  |  |  |  |

Largest members (text+rodata+unwind+data+bss+tdata+tbss), top 15:

| Member | text | rodata | unwind | data | bss | tdata | tbss |
|---|---:|---:|---:|---:|---:|---:|---:|
| `__libc_lgamma_core.o` | 4002 | 360 | 0 | 0 | 0 | 0 | 0 |
| `__libc_log_core.o` | 1328 | 1472 | 0 | 0 | 0 | 0 | 0 |
| `cpow.o` | 2718 | 16 | 0 | 0 | 0 | 0 | 0 |
| `__libc_erfc_table.o` | 864 | 1824 | 0 | 0 | 0 | 0 | 0 |
| `__libc_atan2_core.o` | 1868 | 528 | 0 | 0 | 0 | 0 | 0 |
| `strerror.o` | 24 | 2371 | 0 | 0 | 0 | 0 | 0 |
| `strftime.o` | 1922 | 285 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format.o` | 2042 | 96 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format_float.o` | 1880 | 90 | 0 | 0 | 0 | 0 | 0 |
| `__libc_rem_pio2.o` | 1750 | 160 | 0 | 0 | 0 | 0 | 0 |
| `__libc_exp_core.o` | 880 | 1024 | 0 | 0 | 0 | 0 | 0 |
| `__libc_scan.o` | 1714 | 0 | 0 | 0 | 0 | 0 | 0 |
| `asinh.o` | 1544 | 0 | 0 | 0 | 0 | 0 | 0 |
| `catanh.o` | 1476 | 0 | 0 | 0 | 0 | 0 | 0 |
| `atan2pi.o` | 1468 | 0 | 0 | 0 | 0 | 0 | 0 |
