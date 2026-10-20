# Research: float-float inverse trigonometric functions

Step: P2.05d2. Decision: PLAN D61. It builds on D57 (float-float,
`sp-nearest`), D59 (the corrected square root) and D48 (the double atan2
core).

## Sources reviewed

- P. T. P. Tang's table method, applied to atan as in D48:
  - atan(t) = atan(c) + atan((t − c)/(1 + t·c)) for c from a table;
  - the series only sees the small quotient.
- J.-M. Muller, *Elementary Functions*, 3rd ed., 6.5: the octant reduction
  of atan2, and asin/acos through atan2 with √((1 − x)(1 + x)), where the
  factors are exact and cancellation-free.
- C23 7.12.4 and F.10.1:
  - the atan2/atan2pi special cases (signed zeros, infinities, the
    ±π/4, ±3π/4 and ±¼, ±¾ results);
  - asin/acos of |x| > 1 are NaN;
  - acos(1) = +0, and acospi of 1, −1 and 0 are 0, 1 and ½;
  - asinpi(±1) = ±½, atanpi(±1) = ±¼, atanpi(±∞) = ±½.
- picolibc/newlib (SunPro float atan2: polynomials in float, error near
  1 ulp) and ARM optimized-routines (double arithmetic): descriptions only.

## Design

- **`__libc_atan2f_core(y, x)`**, with float-float arguments:
  - It divides the smaller magnitude by the larger and takes the angle as
    π/2 − a when they were swapped, π − a for a negative x, and the sign
    of y.
  - The unit core picks c = k/16 (atan of k/16 as 17 float pairs,
    136 bytes) and u = (t − c)/(1 + t·c) with |u| ≤ 1/32. t − c is exact
    by Sterbenz and t·c is an exact product.
  - atan u is a series to u⁹ with the cube and −1/3 as float-float.
- **`atanf`** is the core at (x, 1). It returns x below 2^−12 and π/2
  rounded above 2^26.
- **`asinf`, `acosf`** pass the core √((1 − x)(1 + x)): both factors are
  exact TwoSums, the product is float-float and `__libc_ff_sqrt` corrects
  sqrtf by the exact residual.
- **`atan2f`:**
  - the special cases directly;
  - with x > 0 and the operands more than 2^40 apart, y/x rounded once
    (subnormal results included, through `__libc_exp2f_scale`);
  - otherwise the core on operands scaled by the larger one's binade.
- **The π-scaled forms** multiply by 1/π as float-float and return the
  exact cases directly.
  - Below 2^−19, `asinpif`/`atanpif` return x/π rounded once.
  - The first version used 2^−12, the asin/atan cut-off. There the x³
    term is still 2^−25.6 of the result, which gave 0.709 ulp; the test
    caught it.

## Accuracy

`test/unit/float_accuracy`, x86_64:
- 2 097 152 arguments per function;
- every float in [0.984, 1.016] (atan, where the table and the octant
  swap meet) or in [0.984, 1] for asin and [−1, −0.984] for acos, where
  1 − x² cancels;
- `atan2f`/`atan2pif`: both signs, any binades and close magnitudes, plus
  every pair of special values against the double functions.

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `atanf` | 0.500 | 0 of 2 490 369 |
| `asinf` | 0.500 | 0 of 2 359 297 |
| `acosf` | 0.500 | 0 of 2 359 297 |
| `atanpif` | 0.500 | 0 of 2 490 369 |
| `asinpif` | 0.500 | 1 of 2 359 297 |
| `acospif` | 0.500 | 0 of 2 359 297 |
| `atan2f` | 0.500 | 0 of 2 097 152 |
| `atan2pif` | 0.500 | 0 of 2 097 152 |

## Fault injection

Nine mutants (x86_64):
- caught:
  - a table entry's low part with the wrong sign;
  - π − a skipped for a negative x;
  - the tiny-quotient shortcut from 2^−10;
  - `asinf`'s 1 − x as a rounded float;
  - `__libc_ff_sqrt` with the wrong correction;
  - `atanpif`'s ½ from 2^20;
  - the x/π shortcut from 2^−12 (the first version).
- benign:
  - k truncated instead of rounded, which only widens |u| to 1/16, still
    inside the series' accuracy;
  - −1/3 without its low part (marginal, as for −1/6 in D60).
