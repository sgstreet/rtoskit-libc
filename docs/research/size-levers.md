# Research: global build and structure levers

Step: P2.02. Decision: PLAN D52. Measurements: `tools/scripts/size-report`
(D51), release builds. Each flag variant was built in a scratch output tree
(`OUTPUT_ROOT`) with the flag added to `CROSS_FLAGS`, and compared with a
scratch build without it.

## Sources reviewed

- GCC manual, *Optimize Options*:
  - `-Oz` (GCC 12+): optimise for size aggressively, even at a cost in
    speed.
  - `-fipa-pta`, `-fno-jump-tables`, `-fno-inline-small-functions`.
  - `-fno-unwind-tables`, `-fno-asynchronous-unwind-tables`.
  - `-flto`, `-ffat-lto-objects`.
- GCC manual, *ARM Options*: `-mslow-flash-data` avoids literal-pool loads
  (MOVW/MOVT instead). The patch that added it
  (<https://gcc.gnu.org/ml/gcc-patches/2013-11/msg00508.html>) restricts it
  to Armv7-M and states the code-size cost. An LLVM commit measured literal
  pools 4.9 % smaller than MOVW/MOVT in Thumb-2.
- `-fshort-enums` changes `Tag_ABI_enum_size` in the Arm build attributes.
  binutils warns when objects disagree ("uses variable-size enums yet the
  output is to use 32-bit enums"), and every enum in a public header changes
  size: an ABI break for code built without the flag (D15).
- LTO and C libraries:
  - U-Boot's "make memcpy() ... visible for LTO" series
    (<https://lists.denx.de/pipermail/u-boot/2021-May/450468.html>): LTO
    renames or drops library definitions that the compiler later references
    implicitly.
  - The picolibc 1.8.12 that ships with this toolchain is built slim-LTO and
    does not link here (P2.01).
- musl `src/exit/exit.c` (<https://git.musl-libc.org/cgit/musl/tree/src/exit/exit.c>),
  structure only:
  - `exit` calls `__funcs_on_exit` and `__stdio_exit` through weak aliases
    of an empty function.
  - `atexit.c` and the stdio objects that buffer data provide the real
    definitions, so a program links only what it uses.
- newlib-nano's `_printf_float` (a weak reference that `-u _printf_float`
  resolves) and picolibc's link-time `vfprintf` aliasing are the same idea,
  applied to the formatter. They are P2.03's subject (D13).

## Flags (measured)

Change in `libc.a` code and in the reference images' code (text + rodata +
unwind):

| Flag | x86_64 | Arm (all five) | Decision |
|---|---|---|---|
| `-Oz` instead of `-Os` | `libc.a` −0.6 %, images −0.5 to −1.8 % | identical output on every member | Not adopted: no effect on the targets; x86_64 is reference only |
| `-fno-unwind-tables -fno-asynchronous-unwind-tables` | `libc.a` −28.0 %, images −10.7 to −28.9 % | 0: arm-none-eabi emits no `.ARM.exidx` for C | Adopted on x86_64. C needs no unwind tables, and nothing in the library or its tests unwinds |
| `-fno-common` | — | — | Already on (P1.01) |
| `-fshort-enums` | `libc.a` −0.1 % | 0 | Rejected: an ABI change (`Tag_ABI_enum_size`, every enum in the public headers) for nothing |
| `-flto -ffat-lto-objects`, library and port | the link fails | the link fails | Rejected. The port's `__rtos_*_lock` objects are LTO IR; LTO drops them before the `libc.a` members that need them are extracted |
| `-flto` on `libc.a` only, port without | images −5.7 to −17.6 % | armv6-m images −3.0 to −12.5 %; armv7e-m-dp −2.2 to −9.2 % | Not adopted. An LTO archive is tied to one GCC version (users bring their own compiler until Phase 4); `-fcallgraph-info` stack data disappears (D51); the U-Boot and picolibc failures show the risk. Revisit with the Phase 4 toolchain (Q13) |
| `-mslow-flash-data` (v7e-m, v8-m) | — | does not compile: "accessing thread-local storage is not currently supported"; on the 699 members that compile, +3.9 % code | Rejected: incompatible with `thread_local` (D6) and larger |
| `-fno-plt` | — | — | Irrelevant: everything links statically |
| `-fno-jump-tables` | `libc.a` −0.5 % | `libc.a` +0.1 %, printf images −0.6 to +0.6 % | Rejected: mixed and below 1 % |
| `-fno-inline-small-functions` | +0.2 % on printf images | ±0.1 % | Rejected |
| `-fipa-pta` | −0.8 % on printf images | printf images −0.4 to −0.7 %, the rest 0 | Rejected: below 1 %, and GCC warns of excessive memory and compile time on large units |

The flags are not where the size is. The structural chains below are worth
far more.

## Accidental pull-in

Method: probe programs (`main` returning 0; `abort`; `assert` with `NDEBUG`
undefined; `asctime`; `strtol`; `memcpy`; `snprintf`) and the reference
programs, linked like `test/size`. Each image is split per archive member
from the link map, counting allocated output sections only.

Chains found, and how they were broken (C23 behaviour unchanged):

1. **`exit` → the `atexit` table and its lock.** Every program, through
   `main` returning, linked the 264-byte handler tables, `__libc_exit_run`,
   the atexit lock and, through the lock, the port's futex wait/wake and, on
   armv6-m, `__atomic_compare_exchange`/`__atomic_exchange`.
   - Fix: the `atexit` table now lives with its runner in
     `__libc_atexit_run.c`.
   - `atexit` references the table, which links the runner. `exit` calls
     the runner through a weak reference, as D43's tss destructors do.
   - The `at_quick_exit` table keeps its own object.
2. **`abort` → `raise` → the signal table.**
   - A program that never calls `signal` can only have `SIG_DFL` for
     `SIGABRT`, whose action is `__rtos_abort`.
   - Fix: the handler table moves into `raise.c`, so `signal` links
     `raise`, and `abort` names `raise` weakly (`#pragma weak raise`).
3. **`assert` → `snprintf` → the whole formatter, floating point included.**
   - Fix: `__libc_assert_fail` builds the same message by hand: the same
     text, truncated at 255 characters as before, written in one `fputs` to
     `stderr`.
4. **`asctime`/`ctime` → `snprintf`.** The same fix: the fixed format is
   written directly, with `%3d`, `%.2d` and the year in `long long` as
   before.
5. **`exit` → `fclose` and the stream list, in every stdio program.**
   - Closing the streams that `fopen`/`tmpfile` made is now a static
     destructor in `__libc_file_new.c`, the only object that makes them.
     `flush_at_exit` in `__libc_file_write.c` only flushes, as musl's
     `__stdio_exit` does.
   - The order between the two destructors does not matter: `fclose`
     flushes, and the standard streams stay open.
6. **Flushing at exit → `stdout` and `stderr` both.**
   `__libc_stdio_flush_all` names the standard streams weakly. A stream the
   program never names cannot hold output: `hello-puts` no longer links
   `stderr`, and a program that only asserts no longer links `stdout`.

Not chains, and left alone:
- `puts` → `malloc`: the buffer is allocated at the first transfer (D40),
  and P2.03 revisits it.
- `printf` → the float formatter: D13's archive split, P2.03.
- The port's locks, futexes and `__aeabi_read_tp`: part of the seam.

Probe images (bytes in allocated sections, code and data; before → after):

| Probe | armv6-m | armv7e-m | armv7e-m-dp | armv8-m.main |
|---|---|---|---|---|
| empty `main` | 1388 → 506 | 1160 → 518 | 1160 → 518 | 1140 → 512 |
| `abort` | 1606 → 634 | 1378 → 642 | 1378 → 642 | 1358 → 636 |
| `assert` | 9665 → 3245 | 9397 → 2777 | 9429 → 2777 | 9389 → 2765 |
| `asctime` | 7788 → 1572 | 7580 → 1904 | 7613 → 1904 | 7576 → 1904 |
| `strtol` | 2360 → 1488 | 2368 → 1736 | 2368 → 1736 | 2356 → 1736 |
| `memcpy` | 1564 → 682 | 1312 → 672 | 1312 → 672 | 1296 → 666 |
| `snprintf` | 7461 → 6589 | 7265 → 6633 | 7297 → 6665 | 7257 → 6637 |

What remains for an empty program is the port's startup (about 280 bytes on armv6-m),
`__libc_start_main`, `exit` and the TLS setup.

## Thread-local storage

Per-thread bytes, from the `tdata`/`tbss` of each `libc.a` member (linked
only with its users):

| Object | Bytes | Users | Note |
|---|---:|---|---|
| tss values | 256 | `tss_*` | 32 keys × (value, generation); the generation is what makes a deleted key's stale value invisible (D43) |
| `asctime` result | 64 → 26 | `asctime`, `ctime` | C23 7.29.3.1 defines only years 1000–9999 with fields in range, which is exactly 26 characters. The rest is undefined, now cut short instead of sized for any `int` |
| `struct tm` result | 36 | `gmtime`, `localtime` | Already shared, as 7.29.3p1 allows |
| `tmpnam` buffer | 32 | `tmpnam(NULL)` | `L_tmpnam` |
| `rand` state | 8 | `rand`, `srand` | 64-bit PCG (D33) |
| `errno` | 4 | most programs | |
| `strtok` position | 4 | `strtok` | |

- The reference programs carry 4 bytes of libc TLS (`errno`). `math-mix`
  carries none: `errno` came only through the exit lock path.
- Sharing more is not allowed: `asctime`, `tmpnam` and `strtok` results
  may all be live at once.

## Correctness checks

- libc-test 219/219 and every unit test pass on all six boards, debug and
  release.
- New and changed tests:
  - `test/unit/time` checks `asctime` with single-digit fields (space-padded
    day, zero-padded time).
  - `test/unit/stdio` gains `exit-file`: a file left open with buffered data
    reaches its file at exit. The stdio test now links with
    `--wrap=__rtos_close`, which shows that exit closes the file. On Linux,
    `tmpfile` names are unlinked at open, so closing is not otherwise
    observable.
- Fault injection (x86_64 debug), each caught:
  - `close_at_exit` emptied → `stdio` fails;
  - `asctime` day width 2 → `time` fails;
  - `exit` not running the `atexit` handlers → `exit` fails;
  - the assert text changed → `assert` fails;
  - `abort` not raising → `signal` fails;
  - flushing skipping `stdout` → `stdio` fails.

## Build-system fixes found by the measurements

- `ar cr` updated `libc.a` in place, so the member order followed the
  archive's history. `make clean` never deleted archives. Image layout, and
  so alignment fill, depended on that history: 20 bytes on `math-mix`.
  - The archive rule now deletes the archive first.
  - `clean` removes `*.a`.
- Objects of deleted sources stayed in the build tree, where the archive
  rule's `*.o` wildcard picked them up again. A clean build is needed after
  removing a source file.
