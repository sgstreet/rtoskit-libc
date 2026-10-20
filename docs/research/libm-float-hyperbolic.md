# Research: float-float hyperbolic functions

Step: P2.05d3. Decision: PLAN D62. It builds on D57 (the exp2 core), D58
(the log2 core) and D61 (`__libc_ff_sqrt`).

## Sources reviewed

- W. Kahan's and Muller's (*Elementary Functions*, 3rd ed., 5.2 and 6.6)
  standard formulas:
  - sinh = (e^x − e^−x)/2, cosh = (e^x + e^−x)/2;
  - tanh = (1 − e^−2|x|)/(1 + e^−2|x|);
  - asinh = log(x + √(x² + 1)), acosh = log1p((x − 1) + √((x − 1)(x + 1))),
    atanh = log1p(2x/(1 − x))/2;
  - Taylor series near 0, where the formulas cancel.
- picolibc/newlib SunPro float versions (description only): the same
  formulas on float expm1/log1p with errors near 1 ulp.
- C23 7.12.5 and F.10.2:
  - sinh/tanh/asinh/atanh of ±0 are ±0, and cosh(±∞) = +∞;
  - tanh(±∞) = ±1 and acosh(1) = +0;
  - acosh(x < 1) and atanh(|x| > 1) are NaN;
  - atanh(±1) = ±∞ with divide-by-zero.

## Design

- **`__libc_expf_pair(x, &k, &inverse)`** returns e^|x| = 2^k · v and the
  reciprocal 2^−k/v scaled into v's binade (by 2^−2k), both as
  float-float.
  - sinh and cosh are then v ∓ inverse, times 2^(k − 1), rounded once by
    `__libc_exp2f_scale`.
  - The reciprocal is 2^−2k of v. It is dropped only above k = 20, where it
    is below 2^−40 of the result.
  - The first version dropped it above k = 13 and measured 0.55 ulp at
    x ≈ 9.7. That is still faithful, so the test then gained a 0.52-ulp
    bound.
- **`sinhf`:** below ¼ the odd series to x^9/9!, with the cube and 1/6 as
  float-float. The series' remainder is 2^−45 at ¼. Above ¼, the pair.
- **`coshf`:** the pair everywhere.
- **`tanhf`:**
  - below ⅛, the series to 1382x^11/155925 (cube and 1/3 as float-float);
  - otherwise (1 − t)/(1 + t), where t = e^−2|x| comes from the exp2 core
    and is scaled exactly;
  - ±1 beyond 10, where 2e^−2|x| is below half an ulp of 1.
- **`asinhf`:**
  - below ⅛, the series to x^11 (its x^9 and x^11 terms are needed:
    without them the error at ⅛ is 2^−29);
  - up to 2^20, log2 of |x| + √(x² + 1) as float-float, through the log2
    core;
  - beyond, log(2|x|).
- **`acoshf`:**
  - log2 of 1 + ((x − 1) + √((x − 1)(x + 1))), all as float-float. Near 1
    the log2 core's j = 0 path keeps the relative accuracy;
  - beyond 2^20, log(2x).
- **`atanhf`:** below ⅛, the series to x^13/13. Above, log2 of 1 + 2|x|/(1 − |x|)
  as float-float, times ln 2 / 2.

## Accuracy

`test/unit/float_accuracy`, x86_64:
- 2 097 152 arguments per function;
- every float across the series thresholds ([0.22, 0.28] for sinh/cosh,
  [0.11, 0.14] otherwise);
- every float in [1, 1.008] for acosh.

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `sinhf` | 0.500 | 90 of 5 242 881 |
| `coshf` | 0.500 | 19 of 5 242 881 |
| `tanhf` | 0.500 | 235 of 5 242 881 |
| `asinhf` | 0.500 | 30 of 5 242 881 |
| `acoshf` | 0.500 | 0 of 2 162 689 |
| `atanhf` | 0.500 | 40 of 5 242 881 |

The test now also fails any function whose maximum error reaches 0.52 ulp.
Every function of P2.05c–d measures at most 0.509.

## Fault injection

Seven mutants (x86_64), plus the k > 13 cut-off, which the new 0.52-ulp
bound catches:
- caught:
  - `tanhf`'s ±1 from 8;
  - `acoshf` without the x − 1 term;
  - `asinhf`'s log(2x) from 2^8;
  - `coshf` subtracting the reciprocal.
- benign: three mutants move a series/exp boundary or drop a term worth
  2^−32 to 2^−37 near the edge of its range:
  - `atanhf`'s series up to ¼;
  - `sinhf` through the exponential down to 1/16;
  - `tanhf` without its x^11 term.
