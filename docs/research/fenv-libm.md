# Research: `<fenv.h>` and libm part 1 (exact operations)

Step: P1.13. Decisions: PLAN D7, D15, D22, D44, D45, D46.

## Sources reviewed

- CORE-MATH (<https://core-math.gitlabpages.inria.fr/>), P. Zimmermann et al.:
  correctly rounded functions; the target of later libm steps, and the reason
  to keep the exact operations exact in every rounding direction.
- FreeBSD msun and musl `src/math` (structure only): bit-level `floor`,
  `ceil`, `trunc`, `round` (masking the fraction, adding one unit at the
  integer position for the directed cases), `rint` by adding and subtracting
  2^52, `scalbn` in steps of 2^1023 and 2^-969, `fmod` by long division of the
  significands, `fma` by an exact 106-bit product.
- ARM optimized-routines (<https://github.com/ARM-software/optimized-routines>):
  the FPv5 instructions behind libm on Cortex-M (`VRINT[ANPMZRX]`,
  `VMAXNM`/`VMINNM`, `VFMA`).
- S. Boldo and G. Melquiond, *Emulation of FMA and correctly rounded sums:
  proved algorithms using rounding to odd* (IEEE TC 2008): why a value with
  extra bits and a sticky bit can be rounded once more without a double
  rounding error, and when it cannot (fewer than p + 2 bits).
- IEEE 754-2019: `maxNum`/`minNum` (2008) against `maximum`/`minimum`/
  `maximumNumber`/`minimumNumber`, `totalOrder`, payload operations, the two
  tininess rules (before rounding on Arm, after rounding on x86).
- libc-test `src/math` and `src/common/mtest.c`: table tests in the four
  directions, exception checks (inexact ignored to nearest; underflow and
  inexact ignored in the directed modes), rows skipped for directions whose
  `FE_*` macro is not defined.
- The standard: C23 7.6 (`<fenv.h>`), 7.12 (`<math.h>`), Annex B.5 and B.11.

## `<fenv.h>` (D44)

| Core | Arithmetic | `<fenv.h>` |
|---|---|---|
| x86_64 | SSE for float, double and long double (D7) | Every exception and direction, through MXCSR |
| armv7e-m-dp (FPv5-D16) | FPU for float and double | Every exception and direction, through FPSCR |
| armv7e-m (FPv4-SP), armv8-m.main (FPv5-SP) | FPU for float, libgcc software for double | `FE_TONEAREST` only, `FE_ALL_EXCEPT` 0 |
| armv6-m, armv6s-m | libgcc software | `FE_TONEAREST` only, `FE_ALL_EXCEPT` 0 |

libgcc's software floating point rounds to nearest and records no flags, so
on a core where double arithmetic is software no direction or exception can
be honoured for every type; C defines the macros only when supported, which
is the one `#if` in the header (rule 3 of the conditional-compilation list).
The values are the same for every target: the Arm FPSCR encoding (flags
0x01-0x10, `FE_UPWARD` 1, `FE_DOWNWARD` 2, `FE_TOWARDZERO` 3), translated to
MXCSR on x86_64 (flags one bit higher except invalid; upward and downward
swapped). `fenv_t` and `femode_t` hold the raw register; generic functions
in `libc/fenv/` work through a `fenv-arch.h` per variant (`x86_64`,
`fpv5-dp`, `soft`, selected by `LIBC_FENV`). Bits outside `FE_ALL_EXCEPT`
are ignored (libc-test expects success). `FLT_ROUNDS` follows the register.

## Accuracy and errors (D45)

- The operations of this step are exact (or, for `fma` and the narrowing
  functions, correctly rounded) in every supported direction, with IEEE
  exceptions: verified bit for bit.
- Later steps (exponentials to special functions) aim for correct rounding
  of float (evaluated in double) and at most 1 ulp for double to nearest.
- `math_errhandling` is `MATH_ERREXCEPT`: domain and range errors raise
  exceptions and never set `errno` (as musl; on cores without exceptions they
  are not observable). Exceptions are raised by arithmetic in libm, so the
  same code is right on every core.
- libm is compiled with `-frounding-math -fsignaling-nans -fno-math-errno`.
  GCC still compiles `isnan` as an ordered-unordered compare, which raises
  invalid for a signaling NaN; the quiet functions (`getpayload`) test bits.

## Implementation

- Rounding to integers: bit masks, no inexact (`floor`, `ceil`, `trunc`,
  `round`, `roundeven`); `rint` adds and subtracts 2^52 and restores the
  sign of zero; `nearbyint` saves and restores the flags; the float versions
  go through double, which is exact.
- `fromfp` family: one core rounding in the requested math direction, then
  a width check with exact limits; out of range is a domain error (NaN,
  invalid).
- Remainders: one long-division core gives `fmod`, `remainder` and `remquo`
  (low 31 quotient bits), exact including subnormal results.
- `fma`, `fmaf` and the narrowing functions (`fadd` ... `fsqrt` to float,
  `dsqrtl`): an exact value (128-bit significand, exponent, sticky bit) built
  by multiplication, aligned addition (operands normalised to bit 125, so a
  shift below 20 bits loses nothing and a sticky bit never meets massive
  cancellation), long division or a digit-by-digit square root; then one
  rounding. A normal result keeps the target's precision and codes the rest
  as a quarter, half or three quarters of a unit, so one hardware addition
  rounds in the current direction and raises inexact; `scalbn` scales
  exactly or overflows correctly. A subnormal result is rounded by hand from
  `FLT_ROUNDS` with underflow raised when inexact (tininess before rounding:
  Arm's rule; an x86 FMA would differ only when the result rounds up to the
  smallest normal). The `d` narrowing functions are plain operations, long
  double being double.
- `fmax`/`fmin` follow `maxNum`/`minNum` (a signaling NaN gives a NaN with
  invalid), matching `VMAXNM`/`VMINNM`; the `fmaximum` family follows the
  2019 operations. `nan()` returns the default quiet NaN, as `strtod` ignores
  NaN payloads. `totalorder`, `getpayload`, `setpayload`, `setpayloadsig`
  are declared under `__STDC_WANT_IEC_60559_EXT__`.

## FPU layers (D46)

`libc/math/<layer>/` replaces generic files of the same name
(`libc/math/subdir.mk` filters them out); `LIBC_MATH_LAYERS` in the
toolchain file lists the layers of a core: `fpv4-sp` (`fabsf`, `fmaf`),
`fpv5-sp` (float `VRINT*`, `VMAXNM`/`VMINNM`, `VFMA`, `VABS`), `fpv5-dp`
(the same for double; armv7e-m-dp has both). libm lives in `libc.a`; an
empty `libm.a` keeps `-lm` working.

## Tests and vectors

- libc-test: every `math/` test of this step's functions, `math/fenv`,
  `math/fpclassify`, `math/isless` and `regression/fpclassify-invalid-ld80`
  pass on all six boards (87 math tests; `api/math` needs every function,
  P1.16).
- `test/unit/libm/vectors.inc`: 6480 vectors for `fma`, `fmaf`, `ffma`,
  `fadd`, `fsub`, `fmul`, `fdiv`, `fsqrt`, `dsqrtl` in the four directions,
  with inexact, underflow and overflow. Generator: a Python script that
  builds the exact result as a `fractions.Fraction` (square roots bracketed
  through an integer square root scaled by 4^1200, with a sticky epsilon),
  rounds it per IEEE 754 for binary32/binary64 with tininess before
  rounding, and covers random operands, cancellation, sticky-only addends,
  subnormal and overflowing results, and ties. Regenerate rather than edit.
- `test/unit/libm`: special operands, ties at subnormal precision, the C23
  functions libc-test lacks (fromfp family, nextup/nextdown, fmaximum
  family, roundeven, llogb, classification macros, iseqsig, canonicalize,
  total order, payloads, remainder ties, lrint width), and `<fenv.h>`'s
  C23 additions.
- `test/unit/fpu_math` (cores with layers): each instruction version
  against the generic C version compiled into the test under another name,
  same bits and flags, over floats sampled with a prime stride through the
  whole encoding space and random doubles, in every supported direction.
