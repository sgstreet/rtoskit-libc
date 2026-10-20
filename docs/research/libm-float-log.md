# Research: float-float logarithms for single-precision cores

Step: P2.05c2. Decision: PLAN D58. It builds on D57 (float-float, the
`sp-nearest` layer and the accuracy test) and on D47 (the double log core).

## Sources reviewed

- P. T. P. Tang, "Table-driven implementation of the logarithm function in
  IEEE floating-point arithmetic", ACM TOMS 16(4), 1990: write x = 2^e m,
  pick c near m from a table, and evaluate log(m/c) = log(1 + f) with a
  short series; log c comes from the table as a leading and a trailing
  part. D47 applies this to double.
- The same paper and Muller et al., *Handbook of Floating-Point Arithmetic*
  (2nd ed., 11.3):
  - Tang's scheme needs f = (m − c)/c with full precision;
  - f = m · (1/c) − 1 is exact when 1/c is rounded to a float, the product
    is formed exactly (Dekker) and the table holds −log2 of *that* float
    (not of c). The subtraction of 1 is exact by Sterbenz's lemma.
- ARM optimized-routines `logf`/`log2f` (description only): a 16-entry
  table of 1/c and log c in double, with a degree-4 polynomial in double.
  That needs double hardware, as with the exponentials (D57).
- K. Packard, picolibc float math (D56/D57 sources): the SunPro `logf`
  computes in float with an error near 1 ulp.
- C23 7.12.6.11–7.12.6.17 and F.10.3:
  - `log(±0)` = −∞ (divide-by-zero), `log(1)` = +0, `log(x < 0)` is NaN
    (invalid), `log(+∞)` = +∞;
  - `log1p(−1)` = −∞, `log1p(x < −1)` is NaN, `log1p(±0)` = ±0;
  - `logp1` is the IEEE 754 name of `log1p`.

## Design

- **`__libc_log2f_core(x, tail)`** returns log2(x + tail) as float-float.
  1. **Reduce.** A subnormal x is scaled by 2^23 first. Then x = 2^e · m
     with m in [√½, √2), so log2 values near 1 come from e = 0, with no
     cancellation between e and the table term.
  2. **Pick c.** j = round(32 (m − 1)) ∈ [−9, 13]. The table has 23
     entries (1/c as a float, and −log2 of that float as a pair):
     276 bytes.
  3. **Form f exactly.** f = (m · r − 1) + tail · r / 2^e, with |f| < 0.022.
     The product is Dekker-exact and the − 1 is Sterbenz-exact; the sum is
     a TwoSum, because the tail may exceed the first part's precision.
  4. **Series.** log(1 + f) is a Taylor series to f^7 (`__libc_ff_log1p_series`).
     Its −f²/2 term uses an exact square, which the measurements required
     (below). The result is multiplied by 1/ln 2 as float-float, and
     e + log2(1/r) is added.
  5. The result is good to about 2^−40, relative near 1, where j = 0 makes
     the table term zero.
- **`log2f`** rounds the core once. `log2f(2^n)` = n exactly: m = 1 and
  f = 0.
- **`logf` and `log10f`** multiply the core by ln 2 or log10 2 as
  float-float before the one rounding.
- **`__libc_log2p1f_core(x)`:**
  - |x| < 2^−6: the series on x itself, times 1/ln 2;
  - elsewhere 1 + x is formed exactly as a TwoSum pair and goes through the
    core with its tail.
- **The p1 forms:**
  - `log1pf` multiplies by ln 2 and returns x itself below 2^−25;
  - `log10p1f` multiplies by log10 2;
  - `logp1f` calls `log1pf`;
  - `log2p1f` and `log10p1f` handle |x| < 2^−90 as in D57: x is scaled
    into [1, 2) so the product does not underflow, then rounded once.
- **`__libc_exp2f_scale` (D57) generalised.** It now picks the subnormal
  path from the magnitude of the result (ilogb(v) + exponent). Before, it
  assumed |v| ≥ ½: `log10p1f`'s tiny path has v ≈ 0.43–0.87, which rounded
  a subnormal result twice (0.75 ulp, caught by the test).

## Accuracy

`test/unit/float_accuracy`, x86_64, with the layer compiled in:
- 4 194 304 arguments per function over the whole domain (random bit
  patterns and uniform values);
- for the logs, a sweep of every float in [1 − 2^−7, 1 + 2^−7];
- for the p1 forms, a sweep of the four binades on either side of the
  series threshold.

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `logf` | 0.500 | 2 of 4 390 913 |
| `log2f` | 0.500 | 0 of 4 390 913 |
| `log10f` | 0.500 | 1 of 4 390 913 |
| `log1pf` | 0.500 | 452 of 20 971 521 |
| `logp1f` | 0.500 | 418 of 20 971 521 |
| `log2p1f` | 0.500 | 368 of 20 971 521 |
| `log10p1f` | 0.500 | 409 of 20 971 521 |

- The first version rounded −f²/2. That gave 0.515 ulp and about 1.6 in
  1000 results not correctly rounded around 2^−6.
- The exact square brought every log to 0.500 ulp.
- The Arm boards (16 384 arguments each, real layer) show 0 not correctly
  rounded.

The test's bound on results that are not correctly rounded is now 1 in
10 000 (it was 1 in 1000 in D57). The looser bound let a mutant (the square's low part
dropped, about 0.008 ulp) pass.

## Edge cases

- `log2f(±0)` = −∞. `log2f(−0.0)` = −∞, not NaN.
- `logf(−x)` is NaN. `logf(+∞)` = +∞. `logf(1)` = +0.
- `log2f(2^−149)` = −149 exactly (subnormal scaling).
- `log1pf(−1)` = −∞. `log1pf(x < −1)` is NaN. `log1pf(−0)` = −0.
- `log1pf(FLT_MAX)` uses 1 + x = x with the tail 1.
- `log2p1f`/`log10p1f` of a subnormal x round x/ln 2 (x/ln 10) once.

## Tests and fault injection

Eight mutants (x86_64), all caught:
- a table entry's low part with the wrong sign;
- the √2 renormalisation without the exponent increment;
- the subnormal scaling off by one;
- the core ignoring the tail (the p1 functions catch it);
- the square's low part dropped (caught after the bound was tightened);
- the p1 series used up to 2^−2;
- 1/ln 10's low part dropped (the tiny `log10p1f` path);
- the subnormal-path decision of `__libc_exp2f_scale` off by one binade.
