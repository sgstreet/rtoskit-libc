# Implementation-defined behaviour of rtoskit-libc

This document answers the library items of the C23 checklist in Annex J.3
(N3220), together with the locale-specific items of J.4. It covers phase 1,
step P1.17; the decision behind it is PLAN D50. Items that the compiler
decides (GCC, the toolchain in `tools/makefiles/`) are marked *compiler*.

Some behaviour depends on the operating system the library runs on. That
behaviour goes through the RTOS seam (`include/rtos/rtos-port.h`, D4) and is
marked *port*. For those items the Linux test port (`port/linux`) and the
bare-metal port (`port/none`) are described.

Six targets are covered: x86_64, and armv6-m, armv6s-m, armv7e-m (FPv4-SP),
armv7e-m-dp (FPv5-D16) and armv8-m.main (FPv5-SP). `long double` has the
format of `double` everywhere (D7).

## J.3.6 Floating point

- **Accuracy of `<math.h>` and `<complex.h>`.** The bounds below are for
  rounding to nearest. In the directed modes the bounds are 2 ulp (double)
  and 1 ulp (float).
  - Exact operations are exact in every rounding direction. This covers
    rounding to integers, `fmod`, `remainder`, `remquo`, `fmax` and `fmin`,
    the `fmaximum` family, scaling, `frexp`/`ldexp`, `nextafter` and the
    payload functions (D45).
  - `sqrt`, `fma` and the narrowing functions are correctly rounded in every
    direction.
  - The elementary and special real functions are within 1 ulp for double.
    The worst error seen in the reference vectors is 0.61 ulp (`cbrt`); most
    functions are within 0.51 ulp. Float results are computed in double and
    correctly rounded except within 2^-29 of a midpoint (D47, D48, D49).
  - The complex functions are within 4 ulp normwise for double (the larger
    part sets the ulp) and 2 ulp for float (D49).
- **`FLT_ROUNDS`** follows the current rounding direction: 1 to nearest,
  2 upward, 3 downward, 0 toward zero. It is always 1 where only
  `FE_TONEAREST` exists.
- **`FLT_EVAL_METHOD`** is 0 on every target (*compiler*), so `float_t` is
  `float` and `double_t` is `double`.
- **Floating-point exceptions and directed rounding** exist only on x86_64
  and armv7e-m-dp (D44).
  - Elsewhere only `FE_TONEAREST` is defined and `FE_ALL_EXCEPT` is 0.
  - `__STDC_IEC_559__` and `__STDC_IEC_60559_BFP__` are defined only on
    those two targets (`<stdc-predef.h>`, D50).
  - Annex G, decimal floating point and the interchange types are not
    claimed.
- **`math_errhandling`** is `MATH_ERREXCEPT`. Domain, pole and range errors
  raise exceptions and never set `errno` (D45).
- **Results of domain errors** are a quiet NaN. Pole errors give ±∞, and an
  overflow gives ±∞ or the largest finite value as the rounding direction
  dictates. Underflows return the correctly rounded tiny value or zero,
  with underflow raised. Tininess is detected before rounding (the Arm
  rule) for the values the library rounds itself (D45, D47).
- **`fmod(x, 0)`** returns NaN with invalid (a domain error), as do
  `remainder(x, 0)` and `remquo(x, 0, q)`.
- **`remquo`** stores the low 31 bits of the quotient, with its sign.
- **`feraiseexcept`** raises overflow or underflow without inexact. It
  raises nothing on the cores without exceptions.
- **The representation of `fexcept_t`, `fenv_t` and `femode_t`** is the raw
  status word: the Arm FPSCR encoding, translated to MXCSR on x86_64 (D44).
- **The `nan(...)` and `strtod("NAN(...)")` n-char-sequence** is accepted and
  ignored: the result is the default quiet NaN.
- **`strtod` and `scanf` conversions** are correctly rounded for any number of
  digits. Beyond 800 significant digits a sticky flag keeps the rounding
  correct (D38). `CR_DECIMAL_DIG` is 800 under
  `__STDC_WANT_IEC_60559_EXT__`.
- **`printf` conversions** print exact digits for any precision. Infinity
  is `inf`/`INF` and NaN is `nan`/`NAN`, with a sign when negative or
  requested.

## J.3.12 Library functions

- **`NULL`** expands to `((void *)0)`.
- **Diagnostic of `assert`.** It writes
  `<file>:<line>: <function>: Assertion '<expression>' failed.` and a
  newline to `stderr`, then calls `abort`.
- **Locales.** Only the "C" locale exists. `setlocale` accepts `"C"`,
  `"POSIX"` and `""`, which all name it, and returns a null pointer for any
  other name (D42).
- **Character set.** The `<ctype.h>` classes are those of ASCII in the "C"
  locale. `MB_CUR_MAX` is 1, and multibyte characters are single bytes.
  Wide characters are out of scope (PLAN §1).
- **Signals** (D33). The set is `SIGINT` 2, `SIGILL` 4, `SIGABRT` 6,
  `SIGFPE` 8, `SIGSEGV` 11 and `SIGTERM` 15, numbered as on Linux. Any
  number from 1 to 31 is accepted by `signal` and `raise`; others fail.
  - `raise` runs handlers synchronously. Signals from the system (*port*)
    are not delivered by the library.
  - Before a handler runs, the disposition is reset to `SIG_DFL`; no signal
    is blocked.
  - The default action of `SIGABRT` is `__rtos_abort`. The other signals
    end the program with `_Exit(128 + sig)`. `SIGFPE` is never raised for a
    floating-point exception.
- **Text and binary streams** are identical: no newline translation, no
  padding nulls, and a last line needs no newline. Trailing spaces are kept.
- **Append mode** positions every write at the end of the file (*port*).
- **Truncation, zero-length files, valid file names, opening a file more
  than once, the effect of `remove` on an open file, and `rename` over an
  existing name** are decided by the port's `__rtos_open`, `__rtos_remove`
  and `__rtos_rename` (*port*).
  - On Linux these follow POSIX: names are path names, a file may be open
    several times, `remove` unlinks while the file stays open, and `rename`
    replaces the target.
  - `port/none` has no files: `fopen` fails with `ENOSYS`.
- **Temporary files.** `tmpfile` files are removed when closed and at
  `exit`; after abnormal termination they remain (*port*). `tmpnam` names
  come from `__rtos_tmpname`: Linux uses `/tmp/rtoskit-<pid>-<n>`.
  `TMP_MAX` is 10000 and `L_tmpnam` is 32.
- **Buffering** (D40).
  - `BUFSIZ` is 256. Buffers are allocated at the first transfer.
  - `stdin` and `stdout` are line buffered, `stderr` is unbuffered, and
    other files are fully buffered.
  - `FOPEN_MAX` is 16 and `FILENAME_MAX` is 256.
- **`freopen` without a file name** allows only modes that the stream's
  access already covers.
- **`%p`** prints `0x` and lowercase hexadecimal digits; `scanf("%p")` reads
  the same form back.
- **`%[` ranges.** A `-` between two characters in a scanset denotes the
  range (`a-z`); at the start or end it is literal.
- **`errno` after a failed `fgetpos`, `fsetpos` or `ftell`** is the error the
  port reported (`EBADF`, `ESPIPE`, ...), or `EINVAL` for a stream without a
  position.
- **`strtod`, `strtof`, `strtold`** set `errno` to `ERANGE` on underflow when
  the result is tiny and inexact (C23 7.12.1p6), and on overflow.
- **`malloc(0)` and `calloc(0, n)`** return a unique non-null pointer to a
  minimal block (both allocators, D32). C23 makes `realloc(p, 0)` undefined;
  here it shrinks the block to that minimal size and returns it.
- **`abort`** neither flushes nor closes streams. It raises `SIGABRT`, then
  calls `__rtos_abort`, which ends the process with `SIGABRT` on Linux.
- **`_Exit` and `quick_exit`** do not flush streams. `exit` flushes all
  streams and closes the files.
- **Termination status.** `exit` and `_Exit` pass their status to
  `__rtos_exit`, so `EXIT_SUCCESS` (0) and `EXIT_FAILURE` (1) reach the host
  unchanged on Linux.
- **`system`** returns the value of `__rtos_system` (*port*). Neither port
  has a command processor: `system(NULL)` returns 0, and any command
  returns -1.
- **`getenv`** reads `__rtos_getenv` (*port*): the process environment on
  Linux, and none on `port/none`.
- **Time** (D42).
  - `time_t` and `clock_t` are `long long` seconds and microseconds
    (`CLOCKS_PER_SEC` is 1000000).
  - The `TIME_UTC` epoch is 1970-01-01T00:00:00 UTC.
  - `clock` measures `TIME_ACTIVE`, the processor time of the process
    (*port*). `TIME_MONOTONIC`, `TIME_ACTIVE` and `TIME_THREAD_ACTIVE` are
    supported where the port supports them.
  - The local time zone and daylight saving time follow the POSIX `TZ`
    string, read at every conversion. An unset or invalid `TZ` means UTC.
  - `%Z` prints the zone name from `TZ` (`UTC` by default) and nothing when
    `tm_isdst` is negative.
- **`strerror`** messages are the Linux texts for the POSIX error numbers
  (D20), "No error" for 0, and "Unknown error" otherwise.
- **Threads** (D43).
  - `TSS_DTOR_ITERATIONS` is 4, and there are 32 thread-specific storage
    keys.
  - Thread stacks are 256 KiB on Linux (*port*).
  - When the last thread calls `thrd_exit`, `exit(EXIT_SUCCESS)` runs.
  - `port/none` runs one thread, so `thrd_create` fails with `thrd_error`.
- **`rand`** returns values up to `RAND_MAX` = 0x7fffffff, from a
  permuted 64-bit congruential generator (PCG, D33) whose state is kept per
  thread.
- **`bsearch` and `qsort`** with equal elements return or leave any of them.
  `qsort` is not stable.
- **The conversion of a pointer to `intptr_t`** keeps its bits (*compiler*).

## J.3.13 Architecture

- **`<limits.h>`, `<stdint.h>` and `<float.h>` values** are those GCC
  predefines for the target (`__INT_MAX__` and so on, *compiler*).
  - On x86_64 `long` and pointers are 64 bits; on the Arm targets they are
    32 bits.
  - `char` is signed on x86_64 and unsigned on Arm.
  - `BOOL_MAX` is 1 and `BITINT_MAXWIDTH` is the compiler's.
- **Byte order** is little-endian on every target
  (`__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__`).
- **`max_align_t`** has an alignment of 8 bytes on every target, that of
  `long long` and `long double` (D7).

## J.4 Locale-specific behaviour

There is only the "C" locale.
- The decimal point is `.`.
- `isalpha`, `isprint` and the other classes cover ASCII only.
- `strcoll` compares like `strcmp`, and `strxfrm` copies.
- `strftime` uses English day and month names and the C formats.
