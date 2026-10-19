# Research: `<time.h>`, `<locale.h>`, `strerror` completion

Step: P1.11. Decisions: PLAN D6, D15, D20, D42; question Q6 (resolved).

## Sources reviewed

- H. Hinnant, *chrono-Compatible Low-Level Date Algorithms*
  (<https://howardhinnant.github.io/date_algorithms.html>): days from a
  civil date and back, by moving the start of the year to March 1 (so the
  leap day ends the year) and splitting time into 400-year eras of 146097
  days; exact for every proleptic Gregorian date, no tables, no loops.
- POSIX.1-2024, XBD 8.3 (`TZ`): `std offset [dst [offset] [,rule]]`,
  quoted names `<+05>`, offsets `[+-]hh[:mm[:ss]]` west-positive, rules
  `Jn`, `n` and `Mm.w.d` with an optional `/time` (−167 to 167 hours,
  default 02:00), DST one hour ahead when its offset is missing, and
  implementation-defined rules when `dst` has none.
- musl `src/time/` (kraj/musl mirror; structure only): `__secs_to_tm`
  (400/100/4-year cycles from a March epoch), `__tm_to_secs`, `__tz.c`
  (POSIX `TZ` parsing plus zoneinfo files), `strftime.c` with the POSIX
  flag and width extensions.
- The standard: C23 7.29 (`<time.h>`: `timespec_get`/`timespec_getres`
  with `TIME_UTC` and the optional `TIME_MONOTONIC`, `TIME_ACTIVE`,
  `TIME_THREAD_ACTIVE`; `timegm`, `gmtime_r`, `localtime_r`; `mktime`
  normalisation; `asctime`'s exact algorithm; every `strftime`
  conversion with `E`/`O` in the "C" locale), 7.11 (`<locale.h>`:
  `setlocale`, `localeconv`, the "C" values of `struct lconv`), 7.26.6.3
  (`strerror`).

## `<time.h>`

| Topic | Chosen |
|---|---|
| `time_t` | `long long` seconds on every target (no 2038 limit; one header, D15). `clock_t` is `long long` microseconds, `CLOCKS_PER_SEC` 1000000. |
| Clocks | Seam: `__rtos_clock_gettime(int base, struct timespec *)` and `__rtos_clock_getres(...)` with the `TIME_*` bases; `TIME_ACTIVE` is the processor time `clock` reports, so no separate CPU-time call. Linux: `clock_gettime`/`clock_getres` (the `_time64` calls on Arm) with `CLOCK_REALTIME`, `MONOTONIC`, `PROCESS_CPUTIME_ID`, `THREAD_CPUTIME_ID`. `port/none` has no clock (`-ENOSYS`), so `time` returns −1. |
| Calendar | Days ↔ civil date by the March-year/era method above, in 64-bit arithmetic; `gmtime` covers every `time_t` whose year fits `int`, else `EOVERFLOW` and null. |
| Time zone (Q6) | The POSIX `TZ` string, read at each conversion (C requires `localtime` to behave as if `tzset` had been called): names, offsets, `Jn`/`n`/`Mm.w.d` rules with times. An unset, empty or unparsable `TZ` is UTC ("UTC"); a name without an offset (`"GMT"`) is that name at offset 0; DST without rules uses `M3.2.0,M11.1.0` (implementation-defined, the current US rules). Zoneinfo files are out of scope (no file system needed). |
| `mktime`/`timegm` | Normalise the fields in 64-bit arithmetic (months into years, then days), compute the UTC seconds, then for local time resolve `tm_isdst`: positive or negative as given, −1 by trying standard time and checking the result; finally write the normalised fields back through `localtime_r`/`gmtime_r`. Unrepresentable results: −1 and `EOVERFLOW`. |
| Static results | `gmtime`/`localtime` share one `thread_local struct tm`, `asctime`/`ctime` one `thread_local` buffer (D6, 7.29.3p1). |
| `strftime` | Every C23 conversion in the "C" locale, `E`/`O` accepted and ignored; ISO 8601 week numbers (`%G`/`%g`/`%V`); `%z` from the `TZ` offset selected by `tm_isdst` (empty when it is negative), `%Z` the matching name. The POSIX flag (`0` or `+`) and minimum width for `%C %F %G %Y` are supported too (libc-test checks them). Returns 0 when the result does not fit. |
| `asctime` | The standard's `"%.3s %.3s%3d %.2d:%.2d:%.2d %d\n"` algorithm; values outside the ranges it covers are undefined, so they are clamped to keep the buffer bounded. |

## `<locale.h>`

Only the "C" locale (PLAN §1; multibyte locales are out of scope):
`setlocale(category, locale)` accepts `"C"`, `"POSIX"` and `""` (which
selects the native environment, here also "C") and returns `"C"`; any other
name fails with a null pointer. `localeconv` returns the "C" values of 7.11.2.1
(`"."` and empty strings, `CHAR_MAX` for the numeric fields).

## `strerror`

`<errno.h>` gains the remaining POSIX error numbers with their Linux values
(D20), and `strerror` a message for each; anything else is
`"Unknown error"`.

## Edge cases and test vectors

- `gmtime`/`mktime` round trips at the epoch, 2038-01-19T03:14:07/08,
  year 0, negative years, `INT_MAX` and `INT_MIN` years, and
  `time_t` beyond them (`EOVERFLOW`).
- `mktime` normalisation: month 13, day 0, day −1, hour 25, leap days,
  seconds 60, and `tm_isdst` −1 at the DST gaps and overlaps.
- `TZ` strings: `UTC0`, `EST5EDT,M3.2.0,M11.1.0`, `<+0530>-5:30`,
  `CET-1CEST,M3.5.0,M10.5.0/3`, southern rules (`NZST-12NZDT,M9.5.0,M4.1.0/3`),
  `J60`, zero-based `n`, negative and >24 h rule times.
- `strftime`: every conversion for several dates (ISO week edge cases
  around January 1 and December 31), the buffer-too-small case.
- libc-test `functional/strftime` and `functional/time` with the test shim
  providing POSIX `setenv`, `putenv` and `tzset` (they set `TZ` through the
  Linux port's environment); `api/time` and `api/locale` are skipped
  (POSIX declarations required unconditionally).
