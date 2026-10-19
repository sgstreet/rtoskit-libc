# Research: floating-point formatting (`%a %e %f %g`, `strfrom*`)

Step: P1.08. Decisions: PLAN D7, D13, D28, D38, D39.

## Sources reviewed

- G. L. Steele and J. L. White, *How to Print Floating-Point Numbers
  Accurately*, PLDI 1990 (Dragon4): exact digit generation with big
  integers, both shortest and fixed precision.
- F. Loitsch, *Printing Floating-Point Numbers Quickly and Accurately with
  Integers*, PLDI 2010 (Grisu): shortest output with 64-bit arithmetic, a
  ~1 KiB cached-powers table, and a fallback for the ~0.5 % it cannot decide.
- U. Adams, *Ryū: Fast Float-to-String Conversion*, PLDI 2018, and *Ryū
  Revisited: Printf Floating Point Conversion*, OOPSLA 2019: shortest output
  with ~10 KiB of tables; the printf variant (fixed precision, exact) needs
  tables of roughly 100 KiB, or a smaller but slower variant.
- J. Jeon, *Dragonbox*, 2020: shortest output, tables similar to Ryū.
- musl `src/stdio/vfprintf.c` `fmt_fp` (kraj/musl mirror): the value is
  expanded exactly into an array of base-10^9 words (sized for the widest
  `long double`), the requested digits are taken from it, and the rounding
  decision is made by adding a probe in `long double` arithmetic so the
  current rounding mode applies.
- picolibc tinystdio `dtoa.h`: a Ryū-based engine limited to 17 significant
  digits (`DTOA_MAX_DIG`), with digits beyond that printed as zeros —
  small, but not exact for `%.18f` and longer.
- The standard: C23 7.23.6.1p8 (`a A e E f F g G`), p13 (correct rounding up
  to `DECIMAL_DIG` digits, bounded beyond), Annex F.5 (IEC 60559 binding:
  conversions correctly rounded in the current rounding direction),
  7.24.1.3 (`strfromd`, `strfromf`, `strfroml`).

## What the formats need

- `%e`, `%f`, `%g` take any precision, and libc-test checks exact digits
  well beyond 17 (`%.18f` of π gives `3.141592653589793116`, `%.1022f` of
  2^−1021, `%.0f` of 2^128). Shortest-digit algorithms (Grisu, Ryū,
  Dragonbox) answer a different question (what `%g` without precision does
  in other languages); C's conversions are fixed-precision.
- A binary64 value is m·2^e with m < 2^53, so its exact decimal expansion
  has at most 767 significant digits (m·5^1074 for the smallest
  subnormals) and at most 309 integer digits. P1.07's decimal type already
  holds 800 digits and shifts by powers of two.

## Options

| Approach | Tables | Stack | Exact | Notes |
|---|---|---|---|---|
| Ryū printf | ~100 KiB | small | yes | far too large for an MCU |
| Grisu/Ryū/Dragonbox | 1–10 KiB | small | only up to 17 digits | still need a fallback for longer precisions |
| musl base-10^9 array | none | ~500 B (binary64) | yes | rounding probe needs FP arithmetic |
| picolibc 17-digit engine | Ryū tables | small | no | fails libc-test's exact cases |
| **Exact decimal expansion with P1.07's decimal shifts** | none | ≈ 830 B | yes | slowest for extreme exponents; reuses existing code |

## Chosen design

- The decimal type and its operations move from `libc/stdlib/` into a
  shared module `libc/decimal/` (`libc/internal/decimal.h`):
  `__libc_decimal_shift_left`, `__libc_decimal_shift_right` (28 bits per
  pass, 32-bit accumulators, from P1.07), `__libc_decimal_set` (exact
  m·2^e) and `__libc_decimal_round` (keep n digits, rounding in one of the
  four `FLT_ROUNDS` directions with the exact tail). `strtod`'s slow path
  uses the same functions.
- `__libc_format_float()` (the single replaceable symbol, D13) gets the
  running count so it can fail with `EOVERFLOW` before writing, as the
  integer conversions do (D28).
- **`%e`, `%f`, `%g`**: expand |value| exactly (no truncation is possible:
  ≤ 769 digits), round once at the requested position in the current
  rounding direction (Annex F.5), then emit digits straight from the
  decimal with zeros beyond its last digit, so `%.5000f` needs no buffer.
  `%g` rounds to P significant digits first; the exponent X of that
  rounded value picks the style (C23 p8: `%f` with precision P − 1 − X when
  P > X ≥ −4), which rounds at the same digit, so there is no second
  rounding. Without `#`, `%g` drops trailing zeros and a bare point.
- **`%a`**: from the bits; normal and subnormal values are both shown
  normalised with a leading `1` (C23 leaves the leading digit of
  subnormals unspecified; this keeps one code path), zero as `0x0p+0`. With
  no precision, the exact value with trailing zero hex digits removed; with
  a precision, the fraction is rounded in the current direction.
- `inf`/`INF`, `nan`/`NAN` with the sign, the `0` flag not applying; the
  sign of negative zero and of NaN is printed (`-nan`).
- `long double` is binary64 (D7), so `L` shares the path.
- **`strfromd`, `strfromf`, `strfroml`** (C23 7.24.1.3): `snprintf(s, n,
  format, (double)value)`. The format is restricted to `%[.precision]`
  plus one of `aAeEfFgG`; anything else is undefined behaviour, so no
  validation is added.

## Cost

- Code (armv6-m, `-Os`): `__libc_format_float` 1970 B; the shared decimal
  module 770 B (`round` 262, `shift_left` 184, `shift_right` 180, `set`
  144), which `strtod` also uses (its slow path shrank from 866 B to
  528 B); `strfrom*` 16–30 B each.
- Stack: the `struct __libc_decimal` (≈ 830 bytes) in every float
  conversion. Phase 2 may shrink it (base-10^9 limbs, or digits generated
  on demand for `%e`).
- Speed: ordinary values need a few shift passes over a few dozen digits;
  values near the ends of the range need ~40 passes over up to ~770 digits.

## Edge cases

- Rounding carries that add a digit: `%.0f` of 9.5 → `10`, `%.2e` of
  9.995 (exactly 9.9949999…, so `9.99e+00`), `%.3e` of 999999999.75 →
  `1.000e+09`; `%g` where the carry changes the style: `%g` of 999999.5 →
  `1e+06`, of 0.0001 → `0.0001`, of 0.00001 → `1e-05`.
- Ties: exact halves round to even in round-to-nearest (`%.0f` of 0.5 → `0`,
  2.5 → `2`, `%.1f` of 0.25 → `0.2`), and break by any non-zero digit
  beyond the tie.
- `%.0f` of 0.4 → `0`; `%.0e`; `#` with zero precision keeps the point;
  zero padding after the sign; negative zero; huge precision; widths past
  `INT_MAX` fail with `EOVERFLOW` before output.
- Two-digit exponents minimum, three for |X| ≥ 100.

## Generators

The test tables are generated on the host and committed (regenerate
rather than edit):

- `test/unit/printf_float/vectors.inc`: CPython calls glibc's `snprintf`
  through `ctypes` for 58 fixed values (ties, carries, powers of ten, the
  ends of the range, libc-test's cases) plus 30 seeded random ones, each
  with random combinations of the flags `- + space # 0`, widths 0/12/25 and
  precisions none/0/1/3/10/17/20/30 for `e f g E F G`, plain `%e %f %g
  %.0f %.17g`, `inf`/`nan` in every conversion, and `%a`/`%A` forms for
  normal values. glibc's `%a` output with a leading `2` (a rounding carry)
  is skipped, since this library renormalises it.
- `test/unit/printf_float/rounding.inc`: Python's `decimal` module (2000
  digits of precision) quantises the exact value of 34 doubles at keep
  positions −2, −1, 0, 1, 2, 3, 5, 10 and 17 with `ROUND_DOWN`, `ROUND_HALF_EVEN`, `ROUND_CEILING` and
  `ROUND_FLOOR`.

## Test vectors

- libc-test `functional/snprintf` and the regressions `printf-1e9-oob`,
  `printf-fmt-g-round`, `printf-fmt-g-zeros`.
- `test/unit/printf` (extended): a table of format/value/result triples
  generated on the host with an independent correctly rounded
  implementation (CPython's `%` formatting) across flags, widths,
  precisions, ties and extreme values; `%.17g` and `%a` round trips through
  `strtod` for random values; directed rounding through the internal
  `__libc_decimal_round`.
