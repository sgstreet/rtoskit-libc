# Research: float-float exponentials for single-precision cores

Step: P2.05c1. Decision: PLAN D57. It builds on D47 (the double exp/log
cores), D56 (the `sp` layer) and D44 (the soft `<fenv.h>`).

## Sources reviewed

- P. T. P. Tang, "Table-driven implementation of the exponential function
  in IEEE floating-point arithmetic", ACM TOMS 15(2), 1989: reduce
  x = (32m + j) ln2/32 + r, keep 2^(j/32) in a table as a leading and a
  trailing part, and evaluate e^r − 1 with a short polynomial. D47 applies
  this to double. Here it is applied to float with the arithmetic carried
  in pairs of floats.
- T. J. Dekker, "A floating-point technique for extending the available
  precision", Numer. Math. 18, 1971, and D. E. Knuth, TAOCP vol. 2, 4.2.2:
  - TwoSum and Fast2Sum (exact sums);
  - Veltkamp's split (2^⌈p/2⌉ + 1, so 4097 for p = 24) and Dekker's exact
    product. All are exact only when rounding to nearest.
- J.-M. Muller et al., *Handbook of Floating-Point Arithmetic*, 2nd ed.,
  ch. 4 and 14:
  - error bounds of double-word arithmetic;
  - why a final rounding of hi + lo is faithful and almost always correct
    when the pair carries about twice the precision.
- K. Packard, "Picolibc Without Double"
  (<https://www.keithp.com/blogs/picolibc-float/>): newlib/picolibc's
  float-only exp keeps float intermediates (SunPro). Its error is under
  1 ulp but is not correctly rounded as often; the double-intermediate
  variant needs double hardware.
- ARM optimized-routines `expf` (description only,
  <https://github.com/ARM-software/optimized-routines>): computes in double
  with a 2^(j/32) table and a degree-3 polynomial. That design is the right
  one where double is in hardware (armv7e-m-dp keeps our double wrappers),
  and the wrong one on soft double.
- CORE-MATH (<https://core-math.gitlabpages.inria.fr/>): correctly rounded
  float functions computed in double. That is again a double-hardware
  design; it stays the long-term target of D47.
- C23 7.12.6.1–7.12.6.5 and F.10.3: overflow to +∞, underflow toward +0,
  `exp(±0)` = 1, `expm1(±0)` = ±0, `expm1(−∞)` = −1.

## Design

- **Float-float** (`struct __libc_ff` in `internal/math.h`): sum, fast sum,
  split, product, multiply, add, a constant product (`__libc_ff_scale`) and
  the series for e^r − 1. The constants ln 2, 1/ln 2, ln 10, log2 10 and
  log10 2 are float-float pairs, computed with 80 digits and rounded twice
  to float (`tmp`-free: the values are in the header).
- **One core for every exponential, in base 2:**
  `__libc_exp2f_core(z, &exponent)` returns 2^z for a float-float z:
  - k = round(32 z.hi) uses the 1.5 · 2^23 trick (round to nearest);
  - d = (z.hi − k/32) + z.lo is exact (Sterbenz, then Fast2Sum), with
    |d| ≤ 1/64;
  - r = d · ln 2 is a float-float product, |r| < 0.011;
  - e^r − 1 = r + r²(1/2 + r(1/6 + r(1/24 + r/120))), whose truncation is
    below 2^−41;
  - the result is t + t·p with the table entry t = 2^(j/32) (32 pairs,
    256 bytes) and t·p.hi an exact product;
  - the result is good to about 2^−40, before one final rounding.
- **Callers** form z as a float-float:
  - `exp2f`: x;
  - `expf`: x · (1/ln 2);
  - `exp10f`: x · log2 10.

  An integer x makes `exp2f` exact.
- **Scaling, `__libc_exp2f_scale`:** a normal result rounds hi + lo once
  and scales exactly. For a subnormal result:
  - the value is scaled by 2^126, which leaves it below 1 in magnitude;
  - ±1 is added with TwoSum, so the rounding happens on the 2^−149 grid
    (the ulp of [1, 2));
  - the bias is subtracted again. The result is rounded once, never twice.
- **The m1 forms:**
  - near 0 they evaluate the series on r = x, x ln 2 or x ln 10 (as
    float-float), which keeps the relative accuracy;
  - elsewhere they subtract 2^−exponent from the core's pair before the
    one rounding (`__libc_exp2m1f_scale`);
  - below 2^−90, `exp2m1f`/`exp10m1f` scale x into [1, 2) first, so that
    the Dekker product does not underflow, and round once on the way back
    (subnormal results included).
- **Thresholds:**
  - overflow: `expf` 89, `exp2f` 128, `exp10f` 39;
  - zero: −104, −151 and −46;
  - −1: −18, −26 and −8 for the m1 forms.

  Between the thresholds and the true limits the core and the scaling give
  the correctly rounded value, so the thresholds only bound k.
- **Where:**
  - the layer `libc/math/sp-nearest`, after `sp` in `LIBC_MATH_LAYERS` on
    armv6-m, armv6s-m, armv7e-m and armv8-m.main;
  - its makefile refuses a core whose `<fenv.h>` is not `soft`. Float-float
    needs round to nearest, and those cores have no other direction or
    flags (D44);
  - armv7e-m-dp keeps the correctly rounded double wrappers (its double is
    hardware, and it has every rounding direction and the flags);
  - the `sp` symbol check and `-Wdouble-promotion` apply here as well.

## Accuracy (D47 for float on these cores becomes "faithful, almost always correctly rounded")

`test/unit/float_accuracy`, x86_64. It runs 4 194 304 arguments per function
natively, plus every float whose result is subnormal:

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `expf` | 0.500 | 40 of 6 422 529 |
| `exp2f` | 0.500 | 56 of 5 963 777 |
| `exp10f` | 0.500 | 27 of 6 422 529 |
| `expm1f` | 0.506 | 127 of 4 194 304 |
| `exp2m1f` | 0.508 | 168 of 4 194 304 |
| `exp10m1f` | 0.510 | 146 of 4 194 304 |

- The reference is the double function on the widened argument, so a few
  of the counted cases are its own double rounding.
- The Arm boards run 16 384 arguments per function on the real layer and
  agree.

## Edge cases

- `expf(±0)` = 1. `expf(−∞)` = +0 and `expf(+∞)` = +∞. A NaN returns a NaN.
- `expf(x)` for |x| < 2^−25 is 1 + x: e^x rounds to it.
- `exp2f(n)` = 2^n exactly for every integer n in [−149, 127].
- `exp2f(−150)` = 0 (a tie, to even). `exp2f(−149.5)` rounds to 2^−149.
- Subnormal results from `expf` in [−104, −87.3] are rounded once.
- `expm1f(x)` = x for |x| < 2^−25. `expm1f(−18)` = −1.
- `exp2m1f`/`exp10m1f` of a subnormal x are x · ln 2 (x · ln 10), rounded
  once.

## Tests and fault injection

- New `test/unit/float_accuracy`. A function fails if any error reaches
  1 ulp, or if more than one result in a thousand is not correctly
  rounded. The special values must match the double reference exactly.
- On x86_64 the test compiles the `sp-nearest` sources itself (with
  dependency files) and sweeps the subnormal-result ranges.
- Mutants, all caught:
  - a table entry's low part with the wrong sign;
  - `floor(k/32)` replaced by truncation;
  - the subnormal scaling without the TwoSum error term (caught by the
    m1 functions' tiny path after the sweep was added);
  - `exp2m1f_scale` subtracting the wrong power;
  - `expf`'s 1 + x shortcut widened to 2^−12;
  - `exp10m1f` without the tiny-argument scaling;
  - the 1/6 coefficient changed;
  - the Dekker product without its al · bl term;
  - ln 2's low part dropped.
- One rewritten mutant was equivalent (`ilogbf(x) + 1` as the shift just
  scales into [1/2, 1), which the scaling handles).
