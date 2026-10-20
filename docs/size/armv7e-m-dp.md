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

## P2.02 — build and structure levers

Decision D52, `docs/research/size-levers.md`. Before → after, release build.

`libc.a`: code 86545 → 86987 (+442), RAM 788 → 792 (+4), TLS 404 → 366 (-38).

| Program | code | RAM | TLS | libc code | main stack |
|---|---|---|---|---|---|
| hello-puts | 2595 → 2279 | 356 → 156 | 8 → 8 | 1738 → 1464 | 136 → 136 |
| hello-printf-int | 8612 → 8296 | 356 → 156 | 8 → 8 | 6963 → 6689 | 1268 → 1268 |
| hello-printf-float | 8636 → 8320 | 356 → 156 | 8 → 8 | 6963 → 6689 | 1276 → 1276 |
| malloc-heavy | 2024 → 1982 | 228 → 80 | 8 → 8 | 1106 → 1066 | 184 → 184 |
| math-mix | 13708 → 13244 | 200 → 20 | 4 → 0 | 11706 → 11666 | 672 → 672 |

Members that grew, justified:

- `__libc_asctime.o` 294 → 502 (+208): formats by hand instead of calling `snprintf` (on Arm the `asctime` probe image shrinks by 5.7–6.2 KB).
- `__libc_assert_fail.o` 79 → 253 (+174): formats by hand instead of calling `snprintf` (on Arm the `assert` probe image shrinks by 6.4–6.6 KB).
- `__libc_atexit_run.o` 0 → 144 (+144): new: the atexit table (moved from `__libc_atexit_handlers.o`) and its runner.
- `__libc_quick_exit_handlers.o` 0 → 132 (+132): renamed from `__libc_atexit_handlers.o`, now only the `at_quick_exit` table.
- `raise.o` 60 → 188 (+128): now holds the signal table (moved from `__libc_signal_handlers.o`).
- `__libc_file_new.o` 84 → 108 (+24): closes the streams it made at exit (moved from `__libc_stdio_exit.o`).
- `abort.o` 12 → 20 (+8): tests the weak reference to `raise`.
- `__libc_stdio_flush_all.o` 130 → 134 (+4): tests the weak references to `stdout` and `stderr`.
- `exit.o` 44 → 48 (+4): tests the weak reference to `__libc_atexit_run`.
- `__libc_file_write.o` 280 → 282 (+2): calls `__libc_stdio_flush_all` directly.
- Removed: `__libc_atexit_handlers.o`, `__libc_signal_handlers.o`, `__libc_stdio_exit.o`.

The worst stack depth of `exit` drops from 88 to 32 bytes.
