# Size: armv7e-m

Release build of `linux-armv7e-m`, measured with `tools/scripts/size-report` (PLAN D51,
`docs/research/size-measurement.md`). The figures are in `baseline-armv7e-m.json`;
`tools/scripts/size-report diff` compares a build with them. Each phase 2 step
records its reductions, and justifies any growth, in a section below.

## P2.01 baseline (end of phase 1)

`libc.a` (743 members), release build:

| text | rodata | unwind | data | bss | tdata | tbss |
|---:|---:|---:|---:|---:|---:|---:|
| 112594 | 8863 | 0 | 116 | 672 | 8 | 396 |

Reference images (`test/size`): code is text+rodata+unwind, RAM is data+bss, TLS is tdata+tbss per thread; the libc and port shares come from the link map:

| Program | code | RAM | TLS | libc code | libc RAM | libc TLS | port code | libgcc code | main stack |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| hello-puts | 2595 | 356 | 8 | 1738 | 224 | 4 | 818 | 0 | 136* |
| hello-printf-int | 8577 | 356 | 8 | 6931 | 224 | 4 | 818 | 756 | 1268+ |
| hello-printf-float | 10562 | 356 | 8 | 6931 | 224 | 4 | 818 | 2704 | 1284+ |
| malloc-heavy | 2024 | 228 | 8 | 1106 | 144 | 4 | 716 | 0 | 184* |
| math-mix | 23900 | 200 | 4 | 20188 | 132 | 0 | 656 | 2504 | 896* |

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
| `mtx_lock` | 72* | `cnd_wait` | 96* | `exp` | 224* |
| `log` | 296* | `pow` | 416* | `sin` | 328* |
| `cos` | 328* | `tan` | 376* | `atan2` | 784* |
| `lgamma` | 1112* | `tgamma` | 1088* | `erfc` | 456* |
| `cpow` | 1568* | `sinf` | 376* | `expf` | 232* |
| `powf` | 440* |  |  |  |  |

Largest members (text+rodata+unwind+data+bss+tdata+tbss), top 15:

| Member | text | rodata | unwind | data | bss | tdata | tbss |
|---|---:|---:|---:|---:|---:|---:|---:|
| `__libc_lgamma_core.o` | 2920 | 392 | 0 | 0 | 0 | 0 | 0 |
| `__libc_log_core.o` | 1364 | 1472 | 0 | 0 | 0 | 0 | 0 |
| `__libc_erfc_table.o` | 844 | 1824 | 0 | 0 | 0 | 0 | 0 |
| `cpow.o` | 2588 | 16 | 0 | 0 | 0 | 0 | 0 |
| `strerror.o` | 24 | 2371 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format.o` | 2200 | 96 | 0 | 0 | 0 | 0 | 0 |
| `__libc_atan2_core.o` | 1684 | 528 | 0 | 0 | 0 | 0 | 0 |
| `strftime.o` | 1918 | 285 | 0 | 0 | 0 | 0 | 0 |
| `__libc_exp_core.o` | 924 | 1024 | 0 | 0 | 0 | 0 | 0 |
| `__libc_format_float.o` | 1854 | 90 | 0 | 0 | 0 | 0 | 0 |
| `__libc_scan.o` | 1728 | 0 | 0 | 0 | 0 | 0 | 0 |
| `__libc_rem_pio2.o` | 1458 | 160 | 0 | 0 | 0 | 0 | 0 |
| `atan2pi.o` | 1612 | 0 | 0 | 0 | 0 | 0 | 0 |
| `catanh.o` | 1512 | 0 | 0 | 0 | 0 | 0 | 0 |
| `atan2.o` | 1464 | 0 | 0 | 0 | 0 | 0 | 0 |

## P2.02 — build and structure levers

Decision D52, `docs/research/size-levers.md`. Before → after, release build.

`libc.a`: code 121457 → 121899 (+442), RAM 788 → 792 (+4), TLS 404 → 366 (-38).

| Program | code | RAM | TLS | libc code | main stack |
|---|---|---|---|---|---|
| hello-puts | 2595 → 2279 | 356 → 156 | 8 → 8 | 1738 → 1464 | 136 → 136 |
| hello-printf-int | 8577 → 8261 | 356 → 156 | 8 → 8 | 6931 → 6657 | 1268 → 1268 |
| hello-printf-float | 10562 → 10246 | 356 → 156 | 8 → 8 | 6931 → 6657 | 1284 → 1284 |
| malloc-heavy | 2024 → 1982 | 228 → 80 | 8 → 8 | 1106 → 1066 | 184 → 184 |
| math-mix | 23900 → 23460 | 200 → 20 | 4 → 0 | 20188 → 20148 | 896 → 896 |

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

## P2.03 — stdio and printf flavours

Decision D53, `docs/research/stdio-flavours.md`. Before → after, release build, linked against `libc.a` unless the name says otherwise.

`libc.a`: code 121899 → 121933 (+34), RAM 792 → 1292 (+500), TLS 366 → 366 (+0).

| Program | code | RAM | libc code | libgcc code |
|---|---|---|---|---|
| hello-puts | 2279 → 1931 | 156 → 380 | 1464 → 1178 | 0 → 0 |
| hello-printf-int | 8261 → 7981 | 156 → 380 | 6657 → 6437 | 756 → 756 |
| hello-printf-float | 10246 → 9966 | 156 → 380 | 6657 → 6437 | 2704 → 2704 |
| malloc-heavy | 1982 → 1982 | 80 → 80 | 1066 → 1066 | 0 → 0 |
| math-mix | 23460 → 23460 | 20 → 20 | 20148 → 20148 | 2504 → 2504 |
| hello-printf-int-nofloat (new) | 5159 | 380 | 3617 | 756 |
| hello-printf-int-minimal (new) | 4271 | 380 | 3487 | 0 |

Members that grew or are new, justified:

- `__libc_stdin.o` 36 → 288 (+252): holds its static `BUFSIZ` buffer (`.bss`), so reading `stdin` needs no allocator.
- `__libc_stdout.o` 36 → 288 (+252): holds its static `BUFSIZ` buffer (`.bss`), so writing to `stdout` needs no allocator; the heap buffer it replaces was not counted.
- `__libc_format_digits.o` 0 → 194 (+194): new: the digit conversion, moved out of `__libc_format.o`.
- `__libc_format_argument.o` 0 → 172 (+172): new: the argument fetch, moved out of `__libc_format.o` so the minimal flavour can replace it.
- `__libc_scan_store.o` 0 → 62 (+62): new: the integer store, moved out of `__libc_scan.o`.
- `__libc_format_count.o` 0 → 58 (+58): new: `%n`, moved out of `__libc_format.o`.
- `setvbuf.o` 114 → 154 (+40): tracks a caller's buffer and keeps the stream's own one.
- `__libc_file_setup.o` 70 → 76 (+6): tests the weak reference to `malloc`.
- `__libc_scan_count.o` 0 → 6 (+6): new: `%n` for `scanf`, which calls the store.
- Reduced: `__libc_format.o` 2296 → 1938, `__libc_scan.o` 1728 → 1604, `__libc_file_link.o` 44 → 36, `__libc_file_getc.o` 128 → 124, `__libc_scan_float.o` 1105 → 1101, `__libc_stderr.o` 36 → 32, `feof.o` 26 → 24, `ferror.o` 26 → 24, `ungetc.o` 78 → 76.
- The RAM of the stdio images grows by the static `stdout` buffer, which used to come from the heap at the first write; the FILE objects shrink by a pointer each.

## P2.04 — stdlib, allocator, string and time

Decision D54, `docs/research/stdlib-time-size.md`. Before → after, release build.

`libc.a`: code 121933 → 121548 (-385), RAM 1292 → 1292 (+0), TLS 366 → 366 (+0).

| Program | code | RAM | libc code | libgcc code |
|---|---|---|---|---|
| hello-puts | 1931 → 1931 | 380 → 380 | 1178 → 1178 | 0 → 0 |
| hello-printf-int | 7981 → 7333 | 380 → 380 | 6437 → 6545 | 756 → 0 |
| hello-printf-float | 9966 → 9318 | 380 → 380 | 6437 → 6545 | 2704 → 1948 |
| malloc-heavy | 1982 → 1982 | 80 → 80 | 1066 → 1066 | 0 → 0 |
| math-mix | 23460 → 23460 | 20 → 20 | 20148 → 20148 | 2504 → 2504 |
| hello-printf-int-nofloat | 5159 → 4511 | 380 → 380 | 3617 → 3723 | 756 → 0 |
| hello-printf-int-minimal | 4271 → 4271 | 380 → 380 | 3487 → 3487 | 0 → 0 |
| malloc-heavy-tiny (new) | 1922 | 76 | 1006 | 0 |

Members that grew or are new, justified:

- `__libc_udivmod64.o` 0 → 98 (+98): new: the shared 64-bit division by a small divisor that replaces libgcc's.
- `__libc_strtoint.o` 400 → 412 (+12): computes the overflow cutoff with the helper.
- `strftime.o` 2203 → 2215 (+12): calls the division helper; `%C` keeps truncating division.
- `__libc_tz_is_dst.o` 562 → 572 (+10): floor division through the helper.
- `__libc_format_digits.o` 194 → 202 (+8): calls the division helper.
- `__libc_decimal_set.o` 146 → 148 (+2): calls the division helper.
- Reduced: `strerror.o` 2395 → 1968, `__libc_secs_to_tm.o` 496 → 418, `__libc_asctime.o` 502 → 484, `__libc_strtofloat.o` 1131 → 1127.
- The tiny allocator's objects are not in `libc.a`; they replace nano's in `libc-tinymalloc.a` (`malloc-heavy-tiny`).

## P2.05a — soft-float helper audit

Decision D55, `docs/research/libm-softfloat.md`. Before → after, release build.

`libc.a`: code 121548 → 120404 (-1144), RAM 1292 → 1292 (+0), TLS 366 → 366 (+0).

| Program | code | RAM | libc code | libgcc code |
|---|---|---|---|---|
| hello-puts | 1931 → 1931 | 380 → 380 | 1178 → 1178 | 0 → 0 |
| hello-printf-int | 7333 → 7333 | 380 → 380 | 6545 → 6545 | 0 → 0 |
| hello-printf-float | 9318 → 9318 | 380 → 380 | 6545 → 6545 | 1948 → 1948 |
| malloc-heavy | 1982 → 1982 | 80 → 80 | 1066 → 1066 | 0 → 0 |
| math-mix | 23460 → 23256 | 20 → 20 | 20148 → 19956 | 2504 → 2504 |
| hello-printf-int-nofloat | 4511 → 4511 | 380 → 380 | 3723 → 3723 | 0 → 0 |
| hello-printf-int-minimal | 4271 → 4271 | 380 → 380 | 3487 → 3487 | 0 → 0 |
| malloc-heavy-tiny | 1922 → 1922 | 76 → 76 | 1006 → 1006 | 0 → 0 |

Members that grew or are new, justified:

- `__libc_double_to_i64.o` 0 → 118 (+118): new: the integral double to `int64_t` conversion shared by `llrint`, `llround`, `setpayload` and `setpayloadsig`, without `__aeabi_d2lz`/`__aeabi_d2ulz`.
- `__libc_math_divbyzero.o` 0 → 48 (+48): new: out-of-line special-case result (a constant where `<fenv.h>` records nothing, so no soft division).
- `__libc_math_underflow.o` 0 → 48 (+48): new: out-of-line special-case result (a constant where `<fenv.h>` records nothing, so no soft division).
- `clog.o` 1276 → 1300 (+24): calls the out-of-line divide-by-zero result.
- `__libc_math_invalid.o` 0 → 16 (+16): new: out-of-line special-case result (a constant where `<fenv.h>` records nothing, so no soft division).
- `__libc_pow_integer.o` 796 → 812 (+16): calls `__libc_i64_to_double`.
- `__libc_i64_to_double.o` 0 → 12 (+12): new: as `__libc_u64_to_double`.
- `__libc_math_invalidf.o` 0 → 12 (+12): new: out-of-line special-case result (a constant where `<fenv.h>` records nothing, so no soft division).
- `__libc_u64_to_double.o` 0 → 12 (+12): new: 64-bit conversion; two exact 32-bit conversions on fpv5-dp, the native one elsewhere.
- `exp.o` 288 → 296 (+8): calls an out-of-line exception helper instead of inline arithmetic; layout.
- `tanpi.o` 1440 → 1448 (+8): calls an out-of-line exception helper instead of inline arithmetic; layout.
- `lgamma.o` 920 → 924 (+4): calls an out-of-line exception helper instead of inline arithmetic; layout.
- `strtod.o` 284 → 288 (+4): calls `__libc_u64_to_double`.
- `__libc_math_raise_divbyzero.o` 0 → 2 (+2): new: out-of-line exception raising (empty where `<fenv.h>` records nothing).
- `__libc_math_raise_inexact.o` 0 → 2 (+2): new: out-of-line exception raising (empty where `<fenv.h>` records nothing).
- `__libc_math_raise_invalid.o` 0 → 2 (+2): new: out-of-line exception raising (empty where `<fenv.h>` records nothing).
- `__libc_math_raise_overflow.o` 0 → 2 (+2): new: out-of-line exception raising (empty where `<fenv.h>` records nothing).
- `__libc_math_raise_underflow.o` 0 → 2 (+2): new: out-of-line exception raising (empty where `<fenv.h>` records nothing).
- `difftime.o` 64 → 66 (+2): calls `__libc_i64_to_double`.
- Reduced: 66 members, -1486 bytes; the largest: `__libc_fromfp.o` 322 → 260, `rootn.o` 724 → 668, `__libc_exp_scale.o` 488 → 440, `lround.o` 132 → 88.
