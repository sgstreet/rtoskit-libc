# Research: float-float gamma functions

Step: P2.05e2. Decision: PLAN D64. It builds on D49 (the double lgamma and
tgamma), D57 (the exp2 core), D58 (the log2 core) and D60 (the sin core).

## Sources reviewed

- DLMF 5.7.3: log Γ(1 + z) = −γz + Σ_{k≥2} (−1)^k ζ(k) z^k/k for |z| < 1.
- DLMF 5.11.1: Stirling's series with the Bernoulli numbers. Its terms are
  B_2k/(2k(2k − 1) x^(2k−1)).
- DLMF 5.5.3: the reflection Γ(x)Γ(1 − x) = π/sin(πx), here as
  Γ(x)Γ(−x) = −π/(x sin(πx)).
- DLMF 5.15.1: the derivatives of log Γ are the polygamma functions
  ψ^(k). They give the Taylor coefficients of lgamma at its zeros.
- Lanczos (1964), with Godfrey's coefficients (Boost, glibc `tgamma`):
  rejected again, as in D49. The rational form loses relative accuracy
  near the zeros of lgamma, and needs fitted coefficients.
- picolibc/newlib `lgammaf_r` (SunPro, description only):
  - rational approximations on [0.9, 3.1], polynomials around the minimum
    1.4616 and around 1 and 2, then Stirling;
  - negative x by reflection in float;
  - errors of several ulp near the negative zeros, as in most libms.
- CORE-MATH `lgammaf` (description only): correctly rounded, computed in
  double. It treats the negative zeros of lgamma (−2.457, −2.747, …) as
  special cases, with expansions around each. That is the idea adopted
  here, without double.
- C23 7.12.8.3–4 and F.10.5:
  - lgamma(1) = lgamma(2) = +0;
  - lgamma of 0 and of the negative integers is +∞ (divide-by-zero);
  - lgamma(±∞) = +∞;
  - tgamma(±0) = ±∞ (divide-by-zero);
  - tgamma of a negative integer or −∞ is NaN (invalid);
  - tgamma(+∞) = +∞.

## Design

The coefficients follow from the definitions, as in D49: ζ(k), Bernoulli
numbers and polygamma values at the zeros. They come from mpmath at 200
bits, rounded to float pairs. No polynomial is fitted.

- **`__libc_lgammaf_core(x, &sign)`** returns log|Γ(x)| as float-float
  (about 2^−42) and the sign of Γ(x):
  - **x ≥ 8:** Stirling with four correction terms after 1/(12x), whose
    float-float part is the leading term. The next term is below 2^−45 of
    the result at 8. Above 2^60, x(ln x − 1) is scaled by 2^−64 so that
    the exact products cannot overflow.
  - **[2.5, 8):** Γ(x) = (x − 1)…(x − k) Γ(x − k), down to [1.5, 2.5).
    Every x − i is exact, and the logarithm of the float-float product of
    positive factors is added without cancellation.
  - **[0.5, 2.5):** the ζ series at 1 + z, |z| ≤ ½, to z^44. The first 17
    coefficients are float-float and the rest float (their rounding is
    below 2^−44 of the result). Near 2, log1p(z) is added: log Γ(2 + z) =
    log Γ(1 + z) + log(1 + z). The zeros at 1 and 2 are then exact zeros
    of the series in the exact z.
  - **(−½, ½):** log Γ(1 + x) − log|x|.
  - **Below −½:** the reflection. sin(πx) comes from the sin core after
    the exact reduction 32x = K + d. Γ(−x) comes from the same core.
  - **Near the negative zeros:** the reflection subtracts two terms of
    order 1, so its absolute error of about 2^−40 is far too large when
    log|Γ(x)| is small. On (−3, −2) that is 11 ulp at the worst point.
    - The fix is the Taylor series of log|Γ| at each zero z,
      Σ ψ^(k−1)(z)/k! (x − z)^k, to the seventh power, within
      |x − z| < 2^−8/|ψ(z)| (where |lgamma| < 2^−8).
    - z is stored as three floats, so x − z is exact to 2^−72. c1 and c2
      are float-float, c3–c7 are floats.
    - The truncation is below 2^−50 of the result.
    - The 14 zeros of (−9, −2) are tabulated, two per interval between
      poles: 728 bytes.
    - From −9 down, the zeros lie within 1/n! of the poles. No float is
      within the radius of a zero there (checked up to −30 with mpmath),
      and the radius shrinks like 1/n!.
    - Outside the radii, |lgamma| ≥ 2^−8, and the reflection's relative
      error is below 2^−32.
- **`lgammaf`:**
  - NaN is returned unchanged;
  - ±∞ gives +∞;
  - 0 and the negative integers give +∞ with divide-by-zero;
  - 1 and 2 give +0;
  - otherwise the core is rounded once.
- **`tgammaf`:**
  - the result is ±2^(lgamma/ln 2) through `__libc_exp2f_signed` (D60),
    with the core's sign; one rounding, subnormal results included;
  - overflow above 35.1 (Γ reaches FLT_MAX at 35.0401);
  - ±0 below −50 with the sign of the interval (Γ < 0 on (−2k − 1, −2k));
  - between those bounds the exp2 scaling gives the exact overflow and
    underflow.

## Accuracy

`test/unit/float_accuracy`, x86_64:
- the default run sweeps every float in [−2.75, −2.4375], which holds
  both zeros of (−3, −2);
- separate runs swept every float of the ranges below.

| Function | Arguments | max error (ulp) | not correctly rounded |
|---|---|---:|---:|
| `lgammaf` | samples + sweep | 0.500 | 35 of 3 407 873 |
| `lgammaf` | samples + every float in (−9, −0.5] (five runs) | 0.500 | 66 of 45 088 773 |
| `lgammaf` | samples + every float in [0.5, 16) (two runs) | 0.500 | 63 of 46 137 346 |
| `tgammaf` | samples + sweep near 35 | 0.500 | 26 of 2 359 297 |
| `tgammaf` | samples + every float in (−9, −2] | 0.500 | 262 of 19 922 945 |
| `tgammaf` | samples + every float in [0.5, 4) | 0.500 | 408 of 27 262 977 |

- Before the zero expansions, `lgammaf` reached 11.1 ulp on (−3, −2),
  3.3 ulp around −2.747 and 0.59 ulp near −3.955.
- The Arm boards (8192 arguments, real layer) show 0 not correctly
  rounded. The armv6-m run of the whole test takes 23 s under qemu.

## Edge cases

- `lgammaf(1)` = `lgammaf(2)` = +0. `lgammaf(0)`, `lgammaf(−0)` and
  `lgammaf(−3)` are +∞ (divide-by-zero).
- `lgammaf(2^−149)` = 149 ln 2 rounded once. `lgammaf(FLT_MAX)` = +∞
  (overflow). `lgammaf(−2^23 − 2)` is a pole.
- `tgammaf(−0)` = −∞. `tgammaf(−1)` is NaN (invalid). `tgammaf(35.05)` =
  +∞. `tgammaf(−50.5)` = −0 and `tgammaf(−51.5)` = +0 (Γ < 0 on
  (−51, −50)).
- `tgammaf(n)` = (n − 1)! exactly while that is a float: the result is
  exact, so a 2^−40 approximation rounds to it.

## Fault injection

Fifteen mutants (x86_64):
- caught:
  - ζ(2)/2's low part with the wrong sign;
  - c18 changed;
  - ln(2π)/2's low part dropped;
  - ln π's low part dropped;
  - the zero expansions disabled;
  - the zero's middle part dropped;
  - the wrong zero of an interval;
  - the sign inside the zero range inverted;
  - [1.5, 2.5) without log1p;
  - the reflection's sign inverted;
  - `tgammaf`'s overflow bound at 35.0;
  - the sign of the underflowed zero inverted.
- numerically insignificant:
  - Stirling's 1/(1188x^9) dropped: below 2^−40 of the result, 2^−16 ulp.
  - c7 of the zero expansions dropped: below 2^−43 of the result.
  - Stirling from 4 instead of 8: the dropped terms are under 2^−36
    there, 0.504 ulp at worst.

  The first two are kept for the margin; 8 is kept because it costs
  nothing.
