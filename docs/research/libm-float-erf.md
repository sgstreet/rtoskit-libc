# Research: float-float error functions

Step: P2.05e1. Decision: PLAN D63. It builds on D57 (float-float, the exp2
core and its scaling) and on D49 (the double erf/erfc).

## Sources reviewed

- M. Abramowitz and I. A. Stegun, *Handbook of Mathematical Functions*,
  7.1.5, 7.1.6 and 7.1.14 (also DLMF 7.6.2 and 7.9.2):
  - the alternating Maclaurin series of erf;
  - the series with positive terms,
    erf x = (2/√π) e^−x² Σ 2^n x^(2n+1)/(1·3·…·(2n+1));
  - Laplace's continued fraction,
    erfc x = (e^−x²/√π) · 1/(x + (1/2)/(x + 1/(x + (3/2)/(x + …)))).
- W. J. Cody, "Rational Chebyshev approximations for the error function",
  Math. Comp. 23, 1969, and the SunPro `erff` in picolibc/newlib and musl
  (description only):
  - fitted rational approximations on [0, 0.84], [0.84, 1.25], [1.25, 2.86]
    and beyond;
  - erfc as e^−x² · e^R(x)/x with a split x² to keep the exponent exact;
  - errors near 1 ulp in float, with fitted coefficients that need fitting
    tools to re-derive.
- ARM optimized-routines `erff` (description only): a table of erf and
  its derivative at points r, then a Taylor step from the nearest r;
  computed in float. Fast, but its table runs to 512 entries.
- CORE-MATH `erff`/`erfcf` (description only): correctly rounded, through
  double polynomials. Again a double-hardware design (D57).
- C23 7.12.8.1–2 and F.10.5:
  - erf(±0) = ±0 and erf(±∞) = ±1;
  - erfc(−∞) = 2 and erfc(+∞) = +0;
  - erfc underflows (range error) for large positive x.

## Design

The functions keep D49's choice of coefficients that follow from the
definitions (no fitted polynomials). They also follow D57's precision
argument: float-float to about 2^−42, then one rounding.

- **`__libc_erff_core(x)`, 0 ≤ x < 4:** the series with positive terms.
  - Every term is positive, so the sum cancels nothing at any x. The
    alternating series loses about x²/ln 2 bits, 22 bits at 3.9.
  - It stops once the terms have peaked (n ≥ x²) and fallen below 2^−46
    of the sum: 55 terms at 3.9, 6 at 0.1.
  - Each term is the previous one times 2x²/(2n + 1). The square is an
    exact Dekker product, so the term ratio keeps its full precision.
  - e^−x² comes from the exp2 core on −x² · (1/ln 2) as float-float. The
    product with 2/√π (a float-float constant) completes it.
- **`erff`:**
  - NaN and ±0 return themselves;
  - from 3.92 the result is ±1: erfc(x) < 2^−25 (half an ulp below 1)
    beyond 3.9192;
  - below 2^−19 the result is 2x/√π (the x³ term is under 2^−39 of it).
    x is scaled into [1, 2) first, so the product does not underflow, and
    `__libc_exp2f_scale` rounds once (subnormal results included);
  - elsewhere the core's float-float sum is rounded once.
- **`erfcf`:**
  - below 1, 1 − erf(x) with the core. The subtraction loses at most 3
    bits: erfc(1) ≈ 0.157;
  - from 1 to 10.06, Laplace's continued fraction, evaluated bottom-up in
    float-float with 150/x² + 10 levels (160 at 1, 11 at 10). That is
    enough for 2^−46 at every x;
  - the result is e^−x²/√π divided by the fraction and scaled once by
    `__libc_exp2f_scale`, so a subnormal result rounds once;
  - 2 below −3.9 (erfc(|x|) < 2^−24 there);
  - 0 (with underflow) above 10.06: erfc(10.0542) = 2^−150;
  - 1 − x for |x| < 2^−26, where 2x/√π is below half an ulp of 1.
- **Cost:**
  - The continued fraction costs one float-float division per level, so
    `erfcf` near 1 is the slowest float function of the layer.
  - The 1 − erf range stops at 1. The fraction from 0.5 would need 610
    levels, and 1 − erf up to 2 loses up to 8 bits (0.513 ulp measured).

## Accuracy

`test/unit/float_accuracy`, x86_64. Every float near the point where
`erff` becomes ±1 is swept, and every float in [9.25, 10.25] (the
subnormal results of `erfcf`):

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `erff` | 0.500 | 25 of 2 359 297 |
| `erfcf` | 0.500 | 10 of 3 145 729 |

The Arm boards (8192 arguments, real layer) show 0 not correctly rounded.

The x86_64 release run of the whole accuracy test now takes about 47 s.
That is too close to run-unit's 60 s limit on a loaded machine, so
run-unit reads an optional `<name>.timeout` next to the test's source, and
`float_accuracy.timeout` gives 300 s. The test keeps all its samples.

## Edge cases

- `erff(−0)` = −0. `erff(±∞)` = ±1. `erff(2^−149)` = 2^−149 · 2/√π
  rounded once (to 2^−149).
- `erfcf(−∞)` = 2. `erfcf(+∞)` = +0. `erfcf(0)` = 1.
- `erfcf(10.05)` is 2^−149 (a subnormal result). `erfcf(10.1)` = 0.

## Fault injection

Fourteen mutants (x86_64):
- caught:
  - the series stopping at 2^−20;
  - the low part of x² dropped;
  - the term ratio over 2n + 3;
  - the ±1 threshold at 3.9;
  - the 2x/√π shortcut up to 2^−12;
  - 2/√π without its low part;
  - 50/x² + 10 levels;
  - the fraction's numerators k instead of k/2;
  - the 1 − x shortcut up to 2^−20;
  - the zero threshold at 10.04. It was found by the first threshold,
    10.13: that was too high and survived at 10.1, because an mpmath root
    search in a linear scale had stopped early. It was recomputed on the
    logarithm.
- equivalent:
  - the 1 − erf range ending at 0.25, or at 2 instead of 1. Both
    branches are accurate over the overlap. 1 is the cost and accuracy
    choice described above.
