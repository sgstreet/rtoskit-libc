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

## e3b: trigonometric and hyperbolic functions

- **Helpers (libm `sp-nearest`):**
  - `__libc_ff_round_product(a, b, e)`: 2^e · a · b rounded once.
    - A factor below 2^−60 is scaled by 2^64 first, so sinh of a tiny x,
      or sin of a tiny y, keeps its relative accuracy in its part.
    - A zero factor gives the plain product's signed zero: the float-float
      sums would turn −0 into +0.
    - `__libc_exp2f_times` now ends in it.
  - `__libc_sinhcoshf_core(x)`: sinh x and cosh x as float-float for
    |x| < 16:
    - x and 1 + x²/2 below 2^−20;
    - the odd series of sinh (as `sinhf`) and √(1 + sinh²) below ¼;
    - the exp pair (D62) above.
    - The first version stopped the identity at 2^−12. Its x²/6 relative
      error (2^−27) was enough for `sinhf`'s single rounding, but not for a
      float-float factor: 0.53 ulp parts.
- **`csinhf`, `ccoshf`:**
  - sinh x cos y + i cosh x sin y (and the cosh form), each part one
    rounded float-float product.
  - From |x| = 16, e^−|x| is below 2^−46 of e^|x|. The parts are then
    ±e^|x|/2 · cos y and e^|x|/2 · sin y through `__libc_exp2f_times`, so a
    finite part beyond `sinhf`'s overflow survives without the double
    function's halving.
  - The Annex G special cases mirror the double functions'.
- **`ctanhf`:** (sinh x cosh x + i sin y cos y)/(sinh² x + cos² y), all in
  float-float.
  - This is Kahan's (b r s + i t)/(1 + b s²) multiplied through by cos² y.
    The denominator is a sum of two squares, so it cannot cancel, and
    cos² y of a float y is never below about 2^−60.
  - From |x| = 16 the real part is ±1 and the imaginary part
    4 sin y cos y e^−2|x| (relative error 2e^−2|x| < 2^−44). A zero y keeps
    its sign.
  - The first cutoff, 9, was the right one for the real part alone, but
    Re tanh − 1 reaches ±4e^−2|x| on either side of 1. The imaginary
    approximation needs 16: 10 still left 0.52-ulp parts.
- **`csinf`, `ccosf`, `ctanf`:** the rotations −i csinh(iz), ccosh(iz)
  and −i ctanh(iz), with float versions of the exact rotation helpers
  (`__libc_times_if`, `__libc_times_minus_if`).
- **A bug in the double functions:** beyond |x| = 709, `csinh` and `ccosh`
  (P1.16) applied e^|x|/2 in two halves with `copysign(…, x)`. That forced
  the sign of x on the real (`csinh`) or imaginary (`ccosh`) part and lost
  the sign of cos y (sin y).
  - csinh(710 + 2i) came out with a positive real part, while
    sinh(710) cos 2 < 0.
  - Both now multiply by the sign of x. `test/unit/complex` checks the four
    sign combinations against 0.5 e^710 cos 2 (sin 2).

| Function | normwise max (ulp) | parts max, own ulp | not correctly rounded |
|---|---:|---:|---:|
| `csinhf` | 0.500 | 0.5 | 3 |
| `ccoshf` | 0.500 | 0.5 | 2 |
| `ctanhf` | 0.500 | 0.5 | 5 |
| `csinf` | 0.500 | 0.5 | 2 |
| `ccosf` | 0.500 | 0.5 | 3 |
| `ctanf` | 0.500 | 0.5 | 2 |

Of 262 144 arguments each (x86_64): x in [−110, 110] (ctanh [−20, 20])
and y anywhere, or the reverse for the trigonometric forms.

Fourteen mutants:
- caught:
  - the zero check of the rounded product removed;
  - its prescaling disabled;
  - the sinh identity up to 2^−12;
  - cosh without the reciprocal's low part;
  - `csinhf`'s exp2 path from 8;
  - its sign dropped;
  - `ccoshf`'s factor ½ dropped;
  - `ctanhf`'s denominator with sin² y;
  - its cutoff at 10;
  - its zero-y case removed;
  - the wrong rotation in `csinf`;
  - the wrong rotation in `ctanf`;
  - `ccoshf(±∞ ± i0)` without the sign of zero.
- insignificant: 1/6's low part dropped in the sinh series. That is
  2^−32 relative at ¼ (2^−9 ulp), as for `sinhf` (D62).

## e3c: inverse functions

- **Helpers:**
  - `__libc_csqrtf_core(a, b)` (complex layer): √(a + ib) as float-float
    parts for a float-float a, for √(1 ± z) with 1 ± x exact.
    - The other part divides |b| by 2t rather than halving b, which would
      round a subnormal b.
    - Below 2^−60 the operands are scaled by 2^100 and the parts by 2^−50:
      for x = ±1 and y = 2^−149, y² underflows, while √(iy) is 2^−75.
  - `__libc_clogf_modulus` (complex layer): log|z| as float-float, the
    `clogf` path away from |z| = 1.
  - `__libc_asinhf_core(t)` and `__libc_log1pf_core(t)` (libm layer):
    asinh and log1p of a float-float argument. They use `asinhf`'s
    regions, with the identity below 2^−20, and the log1p series below
    2^−6.
- **`cacosf`:** Kahan's formula with u = √(1 − z), v = √(1 + z), in
  float-float.
  - Re = 2 atan2(Re u, Re v); Im = asinh(Re v Im u − Im v Re u). The two
    products have the same sign, so the difference does not cancel.
  - From |z| = 2^24 it uses atan2(|y|, x) − i copysign(log 2|z|, y), good
    to 2^−48.
  - For |y| < 2^−60 it uses the first-order forms acos x − i y/√(1 − x²)
    and |y|/√(x² − 1) − i copysign(acosh|x|, y) (π for x < −1), exact to
    2^−120. The general path would carry y through subnormal products.
  - x = ±1 with y = ±0 is exact (0 or π, −y).
- **`cacoshf`:** from `cacosf`, as the double function.
- **`casinhf`:** −i asin(iz) with Kahan's asin.
  - From 2^24 it uses copysign(log 2|z|, x) + i atan2(y, |x|).
  - For |x| < 2^−60 it uses x/√(1 − y²) + i asin y, or
    copysign(acosh|y|, x) + i copysign(π/2, y).
  - For |y| < 2^−60 the imaginary part is y / Re √(1 + z²).
  - x = ±0 with |y| = 1 is exact.
- **`catanhf`:** on x ≥ 0 (odd),
  ¼ log1p(4x/((1 − x)² + y²)) + i ½ atan2(2y, (1 − x)(1 + x) − y²),
  in float-float.
  - Where 4x/D or 2y/d is below 2^−60, the parts are x/D and y/d, formed
    from x or y scaled by 2^64. The quotient would otherwise be below
    2^−120, and its low part subnormal: 0.69 ulp before.
  - x = 1 with |y| < 2^−60 gives (ln 2 − ln|y|)/2.
  - From 2^40: Re(1/z) and ±π/2. The first cutoff, 2^60, let the
    squares reach 2^120. Veltkamp's split (×4097) then overflowed to NaN,
    and the atan2 core indexed its table with it (a crash).
- **`casinf`, `catanf`:** by rotation.

| Function | normwise max (ulp) | parts max, own ulp | not correctly rounded |
|---|---:|---:|---:|
| `cacosf` | 0.500 | 0.5 | 0 |
| `casinf` | 0.500 | 0.5 | 0 |
| `catanf` | 0.500 | 0.5 | 0 |
| `cacoshf` | 0.500 | 0.5 | 0 |
| `casinhf` | 0.500 | 0.5 | 0 |
| `catanhf` | 0.500 | 0.5 | 0 |
| `catanhf`, x in [½, 2], \|y\| ≤ 1 | 0.500 | 0.5 | 0 |
| `cacosf` near the cut, \|y\| ≤ 2^−10 | 0.500 | 0.5 | 0 |
| `casinhf` near the cut, \|x\| ≤ 2^−10 | 0.500 | 0.5 | 0 |

Fifteen mutants, all caught:
- the csqrt core's tiny scaling disabled;
- its other part over t;
- the asinh identity to 2^−12;
- log(2t) from 2^4;
- the log1p series to 2^−2;
- `cacosf`'s asymptotic form from 2^12;
- its tiny-y real part dropped;
- the sign at x = 1;
- `casinhf`'s tiny-y path off;
- u and v swapped;
- `catanhf`'s x = 1 path off;
- its asymptotic form from 2^20;
- its first-order real part off;
- `cacoshf`'s sign dropped;
- the modulus' scale dropped.

`asinhf` itself still has its own copy of the asinh regions. Sharing it
with `__libc_asinhf_core` is left to P2.05e4, the kernel-sharing step.
