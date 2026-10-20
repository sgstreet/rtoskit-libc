# Research: float complex functions on the float-float cores

Steps: P2.05e3a–e3c. Decision: PLAN D65. It builds on D49 (the double
complex functions), D57–D62 (the float-float real cores) and D46/D57 (the
layer mechanism).

## Sources reviewed

- W. Kahan, "Branch cuts for complex elementary functions, or much ado
  about nothing's sign bit" (1987):
  - csqrt by t = √((|x| + |z|)/2);
  - the inverse functions through √(1 ± z);
  - catanh with log1p;
  - ctanh as (b r s + i t)/(1 + b s²);
  - the sign-of-zero conventions Annex G adopts.
- T. E. Hull, T. F. Fairgrieve, P. T. P. Tang, "Implementing complex
  elementary functions using exception handling", ACM TOMS 20(2), 1994, and
  "Implementing the complex arcsine and arccosine functions …", ACM TOMS
  23(3), 1997: componentwise error bounds of a few ulps from careful
  real-arithmetic formulas, and where cancellation must be avoided:
  log|z| near |z| = 1, and the smaller part of csqrt.
- musl, FreeBSD msun and picolibc `c*f` (description only):
  - picolibc/newlib's float complex functions call the float real
    functions directly in float (cexpf = expf·cosf, clogf = logf(hypotf)),
    with errors of several ulp componentwise and no protection near
    |z| = 1;
  - FreeBSD's `clogf` evaluates the |z| ≈ 1 case in double.
- C23 7.3 and Annex G (G.6): the special cases, which D49 already follows
  for double and which the float functions mirror one for one. cpow has no
  special cases of its own (G.6.4.1: it may be cexp(w clog z)).

## Design

- **Where:** `libc/complex/sp-nearest`, built where `sp-nearest` is in
  `LIBC_MATH_LAYERS`: armv6-m, armv6s-m, armv7e-m and armv8-m.main.
  - `libc/complex/complex.mk` names it in `SUBDIRS`, and `libc/libc.mk`
    adds it to the archive.
  - `libc/complex/subdir.mk` drops the generic double wrappers that the
    layer replaces.
  - armv7e-m-dp and x86_64 keep the wrappers, evaluated in hardware
    double and rounded part by part.
  - As in the libm layer:
    - the makefile refuses a non-`soft` `<fenv.h>`;
    - it compiles with `-Wdouble-promotion`;
    - the `.checked` rule fails on any double helper (`__aeabi_d*`,
      `__muldc3` …). libgcc's `__mulsc3`, used by `w * clogf(z)`, is
      float-only.
- **Shared real helpers (libm `sp-nearest`):**
  - `__libc_sincosf_core(y)`: sin and cos of a float-float angle as
    float-float. It uses the exact reduction and the sin core (D60). Below
    2^−20 it takes sin y = y and cos y = 1 − y²/2, so a subnormal y stays
    exact.
  - `__libc_exp2f_times(z, m)`: 2^z · m rounded once.
    - z is clamped to ±300; beyond that the result overflows or vanishes
      for any m that is a sine or cosine of a float.
    - A factor below 2^−60 is scaled by 2^64 first: a Dekker product of a
      subnormal loses its low bits.
- **e3a:**
  - `cabsf` is `hypotf`; `cargf` is `atan2f` (both 0.500 ulp, D59/D61).
  - `cexpf`:
    - each part is e^x · (cos y or sin y), from the exp2 core times the
      float-float cosine or sine, with one rounding;
    - a part past expf's overflow threshold stays finite when cos y is
      small enough, with no halving trick;
    - x is clamped to ±200 before the float-float product.
  - `clogf`: the real part is log|z|.
    - For max(|x|, |y|) in [√½, 2], it is log1p(a² + b² − 1)/2. a² − 1 is
      exact (Sterbenz) and the four parts of the two exact squares are
      summed in float-float, so a modulus within an ulp of 1 keeps its
      relative accuracy.
    - For a = 1 and b < 2^−20, it is b²/2 from the square of b scaled into
      [1, 2): unscaled, b² underflows.
    - Elsewhere, ½·log2(a² + b²) on the parts scaled by 2^−scale, plus the
      scale, times ln 2, all float-float.
    - The imaginary part is `atan2f`.
  - `csqrtf`: Kahan's method in float-float on the parts scaled by an even
    power of two. The other part divides the significand of y by 2t, and
    y's exponent is applied in the final scaling, so a tiny y next to a
    huge x is not lost to underflow. Each part is rounded once.
  - `cpowf`: exp(w log z) in base 2.
    - L = log2|z| and θ = arg z are float-float (the log2 and atan2
      cores).
    - The exponent is Re w · L − Im w · θ/ln 2, the phase is
      Re w · θ + Im w · L ln 2, and the parts go through
      `__libc_sincosf_core` and `__libc_exp2f_times`.
    - w = 0, a zero z with a positive real w, non-finite operands and a
      phase of 2^24 or more take the plain formula, as the double function
      does.

## Accuracy (e3a)

`test/unit/complex_float`, x86_64: 262 144 random arguments per function
(random bit patterns of every binade and uniform values), plus
`clogf` with |z| within 8 ulps of 1, against the double functions. The
error is normwise (D49), and componentwise for information:

| Function | normwise max (ulp) | parts max, own ulp | not correctly rounded |
|---|---:|---:|---:|
| `cexpf` | 0.500 | 0.5 | 1 |
| `clogf` | 0.500 | 0.5 | 0 |
| `clogf`, \|z\| ≈ 1 | 0.500 | 0.5 | 0 |
| `csqrtf` | 0.500 | 0.5 | 0 |
| `cabsf` | 0.500 | 0.5 | 0 |
| `cargf` | 0.500 | 0.5 | 0 |
| `cpowf` | 0.500 | 1.95 | 67 |

- D49 allows 2 ulp normwise. Every core measures below 0.52, armv7e-m-dp's
  double wrappers included, so the test's bound is 0.52 normwise.
- Where the layer is linked, the test also bounds each part at 0.52 ulp of
  its own value, and requires at most 1 in 1000 results not correctly
  rounded.
- `cpowf`'s parts are reported only. A w with subnormal parts gives a phase
  made of subnormal products, which float-float cannot hold to relative
  precision. Its normwise error stays at 0.500.
- The first version showed what the componentwise view catches. All of
  these were below the 2-ulp normwise bound:
  - csqrtf: 11% of parts not correctly rounded and up to 2·10^7 ulp in the
    smaller part (y scaled into underflow);
  - cexpf: 38 ulp for a subnormal y (the sine core and the product rounding
    on the subnormal grid);
  - clogf: 1.2 ulp for 1 + i·2^−67.
- The cpowf special operands check w = 0 (1 + i0) and zero z, plus every
  in-range result for w in {±0, ±1, ½, −2, 3}. With a huge or infinite w
  on |z| ≈ 1, neither float-float nor the double reference resolves the
  phase. i^FLT_MAX is exactly 1, yet the double function returns −0 − 0i.
- The Annex G special values (every pair of 0, −0, 1, −1, ½, −2, 3,
  ±2^−149, FLT_MAX, ±∞ and NaN) match the double functions' zeros,
  infinities and NaNs exactly, signs included. `test/unit/complex` (its
  vectors and the Annex G cases) passes on the layer as well.
- The Arm boards (2048 arguments) agree: armv6-m with the layer,
  armv7e-m-dp with the wrappers.

## Fault injection (e3a)

Thirteen mutants (x86_64), all caught:
- the small-angle path up to 2^−8;
- the tiny-factor scaling of `__libc_exp2f_times` disabled;
- `cexpf`'s clamp at 80;
- `clogf`'s near-1 path disabled;
- `clogf`'s a = 1 path disabled;
- the scale dropped from `clogf`;
- the squares' low parts dropped;
- `csqrtf`'s final exponent off by one;
- the modulus' low part dropped;
- `cpowf` without the Im w term of the phase;
- 1/ln 2's low part dropped;
- `cargf` with its arguments swapped;
- `cexpf(−∞ + iy)` without the signs of zero.
