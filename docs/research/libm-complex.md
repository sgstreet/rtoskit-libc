# Research: `<complex.h>` and `<tgmath.h>`

Step: P1.16. Decisions: PLAN D7, D15, D49.

## Sources reviewed

- W. Kahan, *Branch cuts for complex elementary functions, or Much ado about
  nothing's sign bit* (1987). Our implementations use these formulas:
  - csqrt: t = √((|x| + |z|)/2), then y/2t;
  - cacos and casin: with u = √(1 - z) and v = √(1 + z),
    acos z = 2 atan2(Re u, Re v) + i asinh(Im(conj(v)u)), and
    asin z = atan2(Re z, Re(uv)) + i asinh(Im(conj(u)v));
  - catanh: Re = log1p(4x/((1-x)² + y²))/4, evaluated on x ≥ 0 by oddness;
  - ctanh: (βρs + it)/(1 + βs²), with t = tan y, β = 1 + t², s = sinh x and
    ρ = √(1 + s²).
- T. E. Hull, T. F. Fairgrieve, P. T. P. Tang, *Implementing the complex
  arcsine and arccosine functions using exception handling*, ACM TOMS 23(3),
  1997. They are more accurate near the branch points but much longer; we
  keep Kahan's formulas with double-double help where cancellation occurs.
- musl and FreeBSD msun `src/complex` (structure only): the Annex G special
  case tables mirrored by symmetry, and cexp/csinh scaling beyond the
  overflow threshold.
- The standard:
  - C23 7.3 (`<complex.h>`; CMPLX in 7.3.9.3);
  - 7.27 (`<tgmath.h>`, the full list of type-generic macros, the
    narrowing macros);
  - Annex G.6 (special values);
  - 6.5.1.1 (`_Generic`).
- libc-test `api/complex`, `api/tgmath`, `api/math` (prototypes, compiled
  with `-pedantic-errors`) and `functional/tgmath`.

## Decisions

- **Imaginary types are not supported.** GCC has no `_Imaginary`, so
  `imaginary` and `_Imaginary_I` are not defined, as C23 7.3.1 allows.
- **`I` is `_Complex_I`**, defined as `__builtin_complex(0.0f, 1.0f)`. The
  GNU `1.0fi` literal would be an error under `-pedantic-errors`.
- **`CMPLX`/`CMPLXF`/`CMPLXL` use `__builtin_complex`**, so infinite and NaN
  parts survive and the result is an address constant (C23 7.3.9.3).
- **`__STDC_IEC_60559_COMPLEX__` is not claimed** (the compiler's
  predefinition is absent with `-nostdinc`). The functions nevertheless
  follow the Annex G special cases, and the unit test checks them; only the
  accuracy near the branch points is weaker than Hull et al.
- **Float functions** are `(float complex)double_function(z)`. **Long
  double** ones call the double ones (D7).
- **`<tgmath.h>`** uses `_Generic`.
  - `__TG_TYPE(x)` maps an argument to a zero of its corresponding type,
    with integers mapped to double (7.27p7).
  - Several generic arguments are combined by adding these zeros in the
    unevaluated controlling expression, which applies the usual arithmetic
    conversions.
  - A complex argument selects the c-function. `fabs` of a complex value
    selects `cabs`.
  - Non-generic parameters (`frexp`'s pointer, `ldexp`'s exponent,
    `nexttoward`'s long double, `fromfp`'s direction and width, `pown`'s
    integer) do not take part in the selection.
  - The narrowing macros pick the l function for a long double argument
    (f prefix) or always (d prefix).
  - Decimal types are out of scope.

## Algorithms

- **cabs** is hypot; **carg** is atan2. **conj** and **cproj** only move
  bits and handle infinities.
- **cexp** computes e^x cos y + i e^x sin y. y = ±0 keeps the zero, and
  beyond x = 709 e^x is applied as two halves, so finite results past exp's
  overflow threshold survive.
- **clog**:
  - Near |z| = 1 the real part is log1p(a² + b² - 1)/2, with the squares
    exact as double-doubles.
  - Elsewhere it is log(hypot) of the parts scaled by a power of two, plus
    the scale times ln 2.
  - The imaginary part is atan2.
- **cpow** is exp(w log z) with log z and w·log z as double-doubles:
  log|z| from the scaled sum of squares and the log core, and arg z from
  the atan2 core. The D47/D48 exp, sin and cos cores and the reduction then
  produce each part with one scaled rounding. The first version, the plain
  formula, lost up to 60 ulp: an absolute error in a large imaginary part
  becomes a phase error.
- **csqrt** uses Kahan's method on the parts scaled by an even power of two.
- **csinh, ccosh** are sinh x cos y + i cosh x sin y and its counterpart,
  with the G.6.2 special cases first and two-step scaling beyond |x| = 709.
- **ctanh** uses Kahan's formula. t = tan y never exceeds 2^54 for a double
  y, so β cannot overflow. Beyond |x| = 22 it returns ±1 + i·4 sin y cos y
  e^(-2|x|).
- **csin, ccos, ctan, casin, catan** are the rotations by i of csinh, ccosh,
  ctanh, casinh, catanh. The rotations move parts and flip signs exactly.
- **cacos** uses Kahan's formula. 1 ± z is formed part by part, so the
  signs of zero survive.
- **cacosh** takes its real part from |Im cacos z| and its imaginary part
  from Re cacos z with the sign of Im z.
- **casinh** is -i·asin(iz) with Kahan's asin.
- **catanh** uses Kahan's formula evaluated on x ≥ 0, since the function is
  odd. The first version took log1p of values near -1 and lost up to 2^42
  ulp near z = -1.
  - The imaginary denominator (1 - x)(1 + x) - y² is a double-double.
  - Beyond 2^500 the result is Re(1/z) ± iπ/2.
  - catanh(±1 + i0) is ±∞ with divide-by-zero.

## Tests and vectors

- libc-test: `api/complex`, `api/math`, `api/tgmath` (header and linkage
  tests, `-pedantic-errors`) and `functional/tgmath` pass on all six boards.
- `test/unit/complex/vectors.inc` holds 4320 vectors, 120 for each of the 18
  value functions in double and float, generated from 600-bit mpmath with
  both parts rounded. They cover random magnitudes 2^-30 … 2^30, the unit
  circle (clog), and points near ±1 and ±i (the inverse functions).
- The bound is max(|Δre|, |Δim|) ≤ 4 ulp of the larger part for double and
  2 for float.
  - Double: ctan reaches 4 ulp; casin, ccos, casinh, ccosh and ctanh reach
    2 ulp; the rest reach 1 ulp.
  - Float: every function is within 0.5 ulp.
- Also checked:
  - the Annex G special-case tables of cacos, cacosh, casinh, catanh,
    ccosh, csinh, ctanh, cexp, clog and csqrt, with signed zeros,
    infinities and NaNs;
  - CMPLX with infinite and NaN parts, I·I, conj, cproj, creal, cimag;
  - the type `<tgmath.h>` selects for every argument-type combination: real,
    complex, integer, mixed, the non-generic parameters, the narrowing
    macros.
