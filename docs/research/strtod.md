# Research: `strtod`, `strtof`, `strtold`, `atof`

Step: P1.07. Decisions: PLAN D7, D13, D38.

## Sources reviewed

- W. D. Clinger, *How to Read Floating Point Numbers Accurately*, PLDI 1990:
  the fast path (an exact integer significand and an exactly representable
  power of ten need one correctly rounded operation) and AlgorithmR
  (approximate, then correct by exact big-integer comparison).
- D. M. Gay, *Correctly Rounded Binary-Decimal and Decimal-Binary
  Conversions*, Numerical Analysis Manuscript 90-10, AT&T Bell Labs, 1990
  (`dtoa.c` `strtod`): Clinger's method with big integers sized to the input,
  heap allocation (`Balloc`), and a correction loop; large and intricate.
- D. Lemire, *Number Parsing at a Gigabyte per Second*, Software: Practice
  and Experience 51(8), 2021 (arXiv:2101.11408), and N. Tao, *The
  Eisel-Lemire ParseNumberF64 Algorithm*
  (<https://nigeltao.github.io/blog/2020/eisel-lemire.html>): a 64×128-bit
  multiply against a table of truncated 128-bit powers of five for
  10^-342..10^308 (≈ 650 entries × 16 bytes ≈ 10 KiB), with a fallback for
  the rare undecided cases. The fast_float library is the reference
  implementation.
- N. Tao, *ParseNumberF64 by Simple Decimal Conversion*
  (<https://nigeltao.github.io/blog/2020/parse-number-f64-simple.html>):
  the fallback used by Go's `strconv` (since 2008) and Wuffs. Keep up to 800
  decimal digits plus a "truncated" flag, multiply or divide by powers of two
  (at most 60 bits per step, with a 64-bit accumulator) until the value is in
  [½, 1), then multiply by 2^precision and round.
- musl `src/internal/floatscan.c` (kraj/musl mirror): decimal digits in a
  ring of base-10^9 words (128 words for binary64 `long double`, 2048 for
  ld80/ld128), normalised by shifts of up to 2^29, with the final rounding
  done in `long double` arithmetic so the current rounding mode applies;
  hexadecimal floats accumulate into an integer with a sticky bit.
- The standard: C23 7.24.1.5 (`strtod` subject sequence, `HUGE_VAL`,
  `ERANGE`), 7.24.1.2 (`atof`), 7.12.1 (overflow and underflow),
  5.2.5.3.3 (`FLT_ROUNDS`), Annex F.5 (conversions correctly rounded in the
  current rounding direction), 7.12p3–p5 (`HUGE_VAL`, `INFINITY`, `NAN`).

## Options

| Approach | Code and data | Stack | Speed | Notes |
|---|---|---|---|---|
| Gay `dtoa.c` | large, heap | variable | good | allocation in `strtod` is unacceptable here |
| Eisel–Lemire + fallback | ~10 KiB table + fallback | small | fastest | the table alone is bigger than this whole step should be |
| musl `floatscan` | moderate | 512 B (binary64) | good | final rounding needs `long double` FP arithmetic, i.e. soft-float on v6-m |
| Clinger fast path + Simple Decimal Conversion | small, no tables beyond powers of ten up to 10^22 | ≈ 820 B on the slow path only | fast for common inputs, slow fallback | integer-only slow path, any rounding mode |

## Chosen design

A shared core `__libc_strtofloat()` (in `libc/stdlib/`, internal header
`libc/internal/strtofloat.h`) with the target format described by its
precision and exponent width, and thin wrappers:

- `strtod`: binary64; `strtof`: binary32; `strtold`: the binary64 core
  (D7, `LDBL_MANT_DIG == DBL_MANT_DIG` asserted); `atof`: `strtod(nptr, nullptr)`.
- The core returns the IEEE bit pattern as `uint64_t`; the wrappers copy it
  into the floating type. The rounding direction comes from `FLT_ROUNDS`
  (1 nearest, 0 toward zero, 2 upward, 3 downward); the core takes it as a
  parameter so tests can drive every mode.

Steps:

1. **Parse** (C23 7.24.1.5p3–p7, "C" locale): skip `isspace`, optional sign,
   then `INF`/`INFINITY`, `NAN`/`NAN(n-char-sequence)` (case-insensitive; the
   payload is ignored: a default quiet NaN with the sign), hexadecimal
   `0x…[p±d]` or decimal `…[e±d]`. A missing exponent digit leaves the `e`/`p`
   outside the subject; `0x` without hex digits is the subject `0`; no
   subject gives 0 and `endptr = nptr`. Exponents saturate (far beyond any
   finite result) so `1e+1000000` and 40000-digit inputs cannot overflow
   `int`.
2. **Fast path** (Clinger), default rounding only: at most 19 significant
   digits, significand ≤ 2^P (P = 53 or 24) and |exponent| ≤ 22 (10 for
   binary32); one multiplication or division in the target type, which is
   correctly rounded by the hardware (or by libgcc's soft-float on armv6-m,
   which only rounds to nearest, matching `FLT_ROUNDS == 1`). Exactness
   conditions keep it to a single rounding.
3. **Slow path** (Simple Decimal Conversion), in a separate function
   (`__libc_decimal_to_float`) so only it pays the stack: up to 800
   significant digits (beyond that a "truncated" flag), shift by ≤ 28 bits
   until the value is in [½, 1), tracking the binary exponent, then multiply
   by 2^64 and take the integer part as a 64-bit significand with a sticky
   bit for any non-zero remainder. Go and Wuffs shift by up to 60 bits with
   64-bit accumulators; here the accumulators are 32 bits (a digit times
   2^28 plus the carry stays below 2^32), because on the 32-bit targets a
   64-bit division by ten is a libgcc call per digit (`__aeabi_uldivmod`).
   The remainder is taken from the quotient, since `-Os` otherwise calls
   the 32-bit division twice per digit: that alone made 5000 conversions
   of short-but-hard inputs 1.7× faster under qemu-arm on armv6-m
   (15.4 s → 8.9 s). Values with decimal exponent > 310 or < −343 go straight to the
   overflow or tiny case. 800 digits are enough: the exact decimal expansion
   of a binary64 rounding boundary has at most 767 significant digits, so
   input beyond 800 digits only matters as a sticky bit.
4. **Hexadecimal**: up to 16 significant hex digits in a `uint64_t`, the
   rest as a sticky bit, binary exponent adjusted per digit.
5. **Round and pack** (shared by both): from (sign, 64-bit significand,
   binary exponent, sticky) produce the format's bits. Subnormal results
   widen the shift so they round once (no double rounding); a carry out of
   the significand moves into the exponent field naturally
   (`bits = (L - emin) << (P - 1) + mantissa`). Rounding: nearest-even, toward
   zero, upward, downward, on the round bit and sticky bit.
6. **Errors** (C23 7.24.1.5p10): overflow returns ±`HUGE_VAL`
   (±infinity) in round-to-nearest and the largest finite value or infinity
   as the direction dictates, with `ERANGE`. Underflow (7.12.1: tiny before
   rounding and inexact) sets `ERANGE` and returns the correctly rounded
   subnormal or zero. Floating-point exception flags are not raised
   explicitly; that waits for `<fenv.h>` (P1.13).

`<math.h>` appears now with only the 7.12p3–p5 constants (`HUGE_VAL`,
`HUGE_VALF`, `HUGE_VALL`, `INFINITY`, `NAN`), which `strtod`'s contract and
libc-test need; the rest of the header comes with libm (P1.13+).

## Cost

- Code (armv6-m, `-Os`): `__libc_strtofloat` 1105 B, `__libc_decimal_to_float`
  866 B (528 B since P1.08 moved the decimal arithmetic into the shared
  `libc/decimal/` module, D39), `__libc_float_pack` 664 B, `strtod` 276 B (with its 23 powers of
  ten), `strtof` 136 B: about 3 KB, plus libgcc's soft-float multiply and
  divide for the fast path on armv6-m.
- Stack: the slow path's `struct decimal` is ≈ 820 bytes; the fast path and
  hexadecimal input use a few dozen.
- Speed: the fast path is one multiplication or division. The slow path is
  proportional to digits × shifts: inputs such as `1e-300` or `DBL_MIN`
  written with 17 digits take about 1.8 ms each under qemu-arm on armv6-m
  (several milliseconds on a Cortex-M0 at tens of MHz). **Phase 3 item:** a
  middle path for inputs of up to 19 digits with large exponents —
  Clinger's extended-precision method (64-bit significand times a power of
  ten built from a small table, with an error bound, falling back to the
  slow path near a rounding boundary) — or base-10^9 limbs in the slow path.

## Edge cases

- `"-0"`, `"-.00000"` → −0.0; `"0x"` → 0 with `endptr` after `0`;
  `"1e"`, `"1e+"` → 1 with `endptr` before `e`; `"."`, `"+"`, `""` → no
  conversion.
- `"inf"`, `"INFINITY"`, `"infinit"` (→ `inf`, endptr after `inf`),
  `"nan"`, `"nan()"`, `"nan(abc_123)"`, `"nan(abc"` (→ `nan`, endptr after
  `nan`), `"-nan"` (sign bit set).
- Halfway cases at every precision boundary round to even; one more digit
  breaks the tie; the libc-test vectors around 2^−1074·½, 2^−1022 + 2^−1075,
  2^1024 − 2^970.
- Binary32 must not be computed through binary64 (double rounding):
  midpoints between adjacent floats are tested directly.
- 40000-digit significands (libc-test `strtod_long`), huge exponents.
- Subnormal results, the smallest subnormal, results that round up to the
  smallest normal, and `ERANGE` for underflow and overflow only.

## Test vectors

- libc-test `functional/strtod`, `strtod_long`, `strtof`, `strtold` and
  `api/stdlib` (`strtod_simple` needs `sin` and `%.300f`: pending P1.15).
- `test/unit/strtod`: for 48 random binary64 and 48 binary32 values per run
  (an eighth subnormal, extra weight at both ends of the range) plus the
  boundary values, both signs: the exact decimal expansion parses back to
  the same value; the exact midpoint to the successor rounds to even; the
  midpoint minus or plus a tenth of its last digit rounds down or up; the
  midpoint plus a unit in the 801st following digit (dropped on input) and
  in the 800th significant digit (pushed out by the shifts) rounds up; each
  in round-to-nearest through `strtod`/`strtof` and in the three directed
  modes through the internal core. Also fixed cases for hexadecimal,
  special values, `endptr`, `errno`, directed overflow and underflow, and
  40000-digit inputs. The expansions are computed in the test with a small
  decimal multiplier, independently of the library. Injected faults caught:
  ties away from zero, no subnormal rounding, downward rounding ignoring the
  sticky bit, a wrong power of ten in the fast path, a lost hexadecimal
  sticky bit, each of the three places the truncated flag is set, and its
  use in the final sticky bit.
