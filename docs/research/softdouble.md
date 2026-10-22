# Research: the double-precision helpers on the single-precision FPUs

Step: Q24 (found in P4.03/P4.04). Decision: PLAN D99.

## The problem

armv7e-m (FPv4-SP) and armv8-m.main (FPv5-SP) have a single-precision FPU, so every double operation is a call to a run-time ABI helper (RTABI 4.1.2). Until now these came from libgcc's `v7e-m+fp/hard` and `v8-m.main+fp/hard` multilibs:

- add, subtract, multiply, divide, the comparisons and the 32-bit conversions are libgcc's Arm assembly (`_arm_addsubdf3.o`, `_arm_muldivdf3.o`, `_arm_cmpdf2.o`, ...);
- the 64-bit conversions are libgcc's generic C (`_fixdfdi.o` and the like), which call the double arithmetic.

P4.03 and P4.04 measured them against Arm Optimized Routines' `at32` helpers (instructions per operation, armv7e-m; bytes added to an empty program):

| Operation | libgcc | AOR | libgcc size | AOR size |
|---|---:|---:|---:|---:|
| `ddiv` | 583 | 82 | 1060 (with `dmul`) | |
| `d2lz` | 178 | 26 | 1926 | 140 |
| `f2lz` | 141 | 21 | 1654 | 124 |
| `i2d` | 41 | 19 | | |
| `dcmp` | 49 | 20 | | |
| `l2d` | 49 | 30 | | |
| `dadd` / `dsub` | 64 / 67 | 46 | 888 (with the conversions) | |
| `dmul` | 58 | 55 | | |

The 64-bit conversions are slow and large because the generic C splits the value with double multiplications and subtractions. libgcc's division is a bit-at-a-time loop.

### libgcc's addition misrounds

Writing the test for this step turned up a correctness defect. The new unit test (`test/unit/softdp`) folds 30,000 random cases of every operation into one hash, and the expected hash is fixed by x86_64's hardware. On armv8-m.main, which still used libgcc, the hash differed. The four differing results are all additions or subtractions:

| a | b | operation | correct (hardware, exact rational) | libgcc |
|---|---|---|---|---|
| `c0b00000000002f8` | `3eaea989ce649171` | a + b | `c0afffffffe15c66` | `c0afffffffe15c67` |
| `3f6fffffffffff42` | `c170000000000165` | a + b | `c16fffffffe002ca` | `c16fffffffe002cb` |
| `c7f0000000160000` | `ca00000000070000` | a - b | `49fffffffffe0000` | `49fffffffffdffff` |
| `3de8ffd7e235465b` | `3ff0000000000063` | a - b | `bfefffffffe700ee` | `bfefffffffe700ef` |

- **Cross-check:** each correct result was recomputed with Python's `fractions` (the exact rational sum rounded to the nearest double).
- **Where it applies:** armv7e-m's libgcc gives the same wrong results.
- **The common shape:** an effective subtraction with exponents more than one apart, whose result falls one binade below the larger operand. The result is then one ulp off.
- **How often:** 4 of the test's 60,000 additions and subtractions. The test's operands are weighted toward such cases, so random operands meet it less often.

So the replacement is a correctness fix as well as a speed and size one, and it covers armv8-m.main too: its FPU and libgcc are the same.

## Sources reviewed

- **IEEE 754-2019**, the rules the results follow:
  - §4.3 (rounding to nearest, ties to even);
  - §5.4.1 (one correct rounding of the exact result);
  - §5.11 (comparisons and unordered);
  - §6.2.3 (a NaN result carries an input NaN's payload);
  - §6.3 (the sign of zero results);
  - §7.2 (invalid operations: ∞ − ∞, 0 × ∞, 0/0, ∞/∞).
- **C23** §6.3.1.4 and §6.3.1.5, for the conversions:
  - truncation toward zero for floating to integer, undefined out of range;
  - correct rounding for integer to floating and for double to float.
- **Arm Run-time ABI** (IHI 0043, "Run-time ABI for the Arm Architecture"), §4.1.2:
  - the helper names and their semantics;
  - the helpers use the base procedure call standard, so doubles travel in r0–r3 even in a hard-float build. That is GCC's `pcs("aapcs")` attribute.
- **Armv7-M Architecture Reference Manual**, the instructions the fast paths are built on:
  - `UMULL`, `UMLAL` and `UMAAL` (DSP extension: Armv7E-M, and Armv8-M Mainline with `+dsp`, which both boards have);
  - `CLZ`, `RRX`;
  - register shifts, which give 0 for counts of 32 to 255;
  - `IT` blocks.
- **D. E. Knuth, *TAOCP* vol. 2** (3rd ed.):
  - §4.2.1 (floating-point addition and multiplication: alignment, normalisation, one rounding);
  - §4.3.1 Algorithm D: long division with a trial quotient digit from the leading digits. Its error bound, at most 2 too large, is the model for this division's at most 2 too small.
- **J.-M. Muller et al., *Handbook of Floating-Point Arithmetic*** (2nd ed., 2018):
  - chapter 7 (software implementation of binary floating-point: rounding from a guard bit and a sticky bit, subnormal results shifted before the single rounding).
- **J. R. Hauser, *Handling floating-point exceptions in numeric programs*** (ACM TOPLAS 18(2), 1996): a sum or difference of two floating-point numbers that lands in the subnormal range is exact, so the addition never rounds a subnormal result.
- **D. Goldberg, *What every computer scientist should know about floating-point arithmetic*** (1991): guard digits, and Sterbenz's lemma (y/2 ≤ x ≤ 2y makes x − y exact), which makes the large-cancellation path exact.
- **J. R. Hauser, *Berkeley SoftFloat* release 3 documentation** (<http://www.jhauser.us/arithmetic/SoftFloat.html>): the structure of a soft-float library (shared round-and-pack, "jamming" right shifts that keep a sticky bit). Only the documentation was read.
- **libgcc's helpers and AOR's `at32` helpers:** measured only (P4.03, P4.04, and the misrounding above), never read as code (clean-room, PLAN D91).

## Design

A new libm layer, `soft-dp` (`libc/math/soft-dp`), selected by `LIBC_MATH_LAYERS` on armv7e-m and armv8-m.main (D46: a layer may add files). It defines every helper the compiler calls for double arithmetic, so no libgcc floating-point member is linked. Only round to nearest is implemented and no flags are raised: `<fenv.h>` has nothing else on these cores (D44).

**Assembly where speed matters.** First attempt: everything in C (uint64_t arithmetic, 32 × 32 → 64-bit products):

- the C was correct the first time;
- but at `-Os` GCC made `dadd`, `dmul` and `d2f` about twice as long as libgcc's assembly (128, 126 and 43 instructions against 64, 58 and 20);
- that made the double libm 30–90 % slower;
- a fast path in C with 32-bit words reached 72 for `dmul` and no better.

So the operations that dominate the libm are Thumb-2 assembly:

- **`__aeabi_dadd`** (`__aeabi_dsub` flips b's sign and branches to it):
  - the operands are exchanged so that |a| ≥ |b| as bits without the sign. Then only a can be infinite or NaN, and only b zero or subnormal (unless both are), which leaves two tests on the fast path;
  - b's significand is shifted right by the exponent difference d into three words, the third holding the bits shifted out. Below 32 places that is all of them; from 32, the top 32 and a sticky bit, which keeps the two bits below the result exact through a one-place normalisation. d is capped at 63, which leaves b below a quarter of an ulp, a sticky bit;
  - a sum carrying into bit 53 shifts right one place;
  - a difference below 2^52 shifts left one place;
  - a difference below 2^51 has cancelled two or more bits, so the exponents were at most one apart. It is exact (Sterbenz) and is normalised with `CLZ`, never below the exponent of field 1. A result there is subnormal and exact;
  - a subnormal b has no hidden bit and the exponent of field 1; two subnormals add or subtract exactly as integers;
  - rounding: the third word's top bit is half an ulp, and a tie clears the last bit;
  - zeros, ∞ − ∞ and NaN (the larger payload of two, quietened) are handled at the side.
- **`__aeabi_dmul`:**
  - a's significand moved to the top of 64 bits times b's 53 bits gives a 117-bit product from `UMULL`, `UMLAL` and two `UMAAL`s;
  - its upper half is the significand (shifted left once if below 2^52), and its lower half supplies the rounding bit and the sticky bit;
  - a subnormal operand is normalised with `CLZ` first;
  - a result below the range is shifted onto the subnormal grid, with a sticky bit, and rounded once; above the range it is infinity;
  - zero, infinity, 0 × ∞ and NaN are handled at the side.
- **The conversions:** the conversions that run in every libm call or every `printf("%f")` (`__aeabi_d2f`, `__aeabi_f2d`, `__aeabi_d2iz`, `__aeabi_d2uiz`) and the 64-bit to double ones (`__aeabi_l2d`, `__aeabi_ul2d`, through a shared hidden core `__libc_softdouble_from_u64`):
  - `f2d` moves the sign in through the carry (`LSLS` then `RRX`) and adds the bias difference;
  - `d2f` takes the field's low 8 bits with the top one flipped, which is the float field;
  - a `d2f` result outside float's normal range goes to the C `__libc_softdouble_d2f`.

**C where it is enough** (`softdouble.h` holds unpack, pack, compare and the truncation helpers):

- **`__aeabi_ddiv`:**
  - long division of the significands in base 2^18, three digits after a leading 1;
  - each digit is estimated from a 32-bit reciprocal of the divisor (one single-precision `VDIV`; the numerator 2^62(1 − 2^-21) keeps the estimate below the true reciprocal, within 2^-20);
  - each digit is then corrected by the exact remainder. The estimate is at most 2 too small, and the remainder is computed modulo 2^64 but is below 3 × divisor in fact;
  - the final remainder gives an exact sticky bit;
  - 174 instructions, against libgcc's 583.
- **The comparisons:**
  - `__aeabi_dcmp{eq,lt,le,ge,gt}` call one shared ordering, `__libc_softdouble_compare`. It is Thumb-2 assembly since D101; at first it was C (the magnitude bits, negated for a negative sign, compared as one signed integer), which is now the test's model, `test/unit/softdp/reference_compare.c`.
  - The assembly sets the flags as `CMP` would for two ordered integers: Z for equal, C clear for less. Unordered is C set with Z clear, which none of the conditions the callers test (`LO`, `LS`, `EQ`) reads as true. `gt` and `ge` exchange the operands first.
  - On the common path, each operand costs one compare for an exponent field of 2047; only then does it test for a NaN. Same signs compare as unsigned 64-bit integers, reversed for two negatives, and two zeros of one sign come out equal there. Only opposite signs need the test for two zeros.
  - The shared ordering touches only ip, so each predicate is a call and one conditional move.
  - `__aeabi_dcmpun` tests the two NaNs directly, in C, because every `isnan` in the libm calls it.
- **The rest:** `__aeabi_i2d`, `__aeabi_ui2d`, and the truncating 64-bit conversions `__aeabi_d2lz`, `__aeabi_d2ulz`, `__aeabi_f2lz` and `__aeabi_f2ulz`. These are a few integer shifts, against libgcc's 1.6–1.9 KB each.

Out of range, the conversions to integers saturate, and NaN gives 0. That is what the FPU's own `VCVT` does; C leaves the behaviour undefined.

**Strong definitions, and libgcc's callers replaced** (D100, superseding D99's weak definitions). Every helper is a strong global symbol, so a clash with libgcc is a link error, not a silent substitution.

- **The problem:** libgcc's own complex and `powi` helpers (`__muldc3`, `__divdc3`, `__divsc3`, `__powidf2`) call the double helpers. When a program pulls one of them in, GNU ld resolves its references while it scans libgcc, from libgcc's own double objects (`_arm_addsubdf3.o`, `_arm_cmpdf2.o`, `_arm_muldivdf3.o` and the like).
  - Where none of their symbols was already linked from `libc.a`, libgcc's arithmetic, misrounding addition included, came in silently beside ours.
  - Where one was, the link failed: a complex division duplicated `__aeabi_dcmpgt`.
  - The first version of this layer made the helpers weak, which turned that error into a silent substitution.
- **The fix:** the layer defines those four helpers itself, following C23 Annex G.5.2:
  - multiplication by the textbook formula, with the recovery of infinite results when both parts come out NaN;
  - double division with the divisor scaled by a power of two (`logb`/`scalbn`), and the infinity and zero cases;
  - the double division skips the scaling when every part is zero or within 2^-128 to 2^128. There every product, numerator, denominator and quotient stays normal, scaled or not, so the result is the same bits without four library calls;
  - float division in double, where the products of floats are exact and nothing overflows, through one reciprocal and two products;
  - `powi` by squaring.

  `__mulsc3` stays libgcc's: it computes in the FPU's single precision and calls no double helper.
- **Checked:** programs using double and float complex multiplication and division, `cpow`, `__builtin_powi` and a 64-bit conversion link without any libgcc floating-point member on both cores. `test/unit/complex_ops` checks the helpers' Annex G cases, the scaling and their agreement with the textbook formula on every board, against libgcc's versions elsewhere.
- **Speed** (armv7e-m, instructions per call; `docs/perf/armv7e-m.md`): `__muldc3` 371, `__divdc3` 1059, `__divsc3` 882 and `__powidf2` 300.
  - Against libgcc's own helpers on this layer's arithmetic, that is +0.5 %, −8 %, −4 % and equal.
  - Against all-libgcc: 402, 2460, 1812 and 321.
- **What stays libgcc's:** helpers this layer does not define, such as libgcc's fixed-point conversions (`_Fract` types, a GNU extension) or the GNU-named `__adddf3` family. A program that reaches one now either links libgcc's object or fails with a duplicate symbol, which shows where the layer needs to grow.

**The C models.** The general C addition and multiplication written first are kept as the models of the assembly, `test/unit/softdp/reference_{add,multiply}.c`:

- x86_64 checks them against its hardware;
- the soft-dp cores check the assembly against them.

## Edge cases

- **Zeros:** −0 + −0 = −0, +0 + −0 = +0, and x − x = +0. A zero times a finite operand is a zero with the signs' exclusive or.
- **Subnormals:** subnormal operands of every operation; subnormal results, which are exact for addition and rounded once on their own grid for multiplication and division; the rounding from the largest subnormal into the smallest normal.
- **Overflow:** to infinity, including through the rounding carry from the largest finite value.
- **Invalid operations:** ∞ − ∞, 0 × ∞, 0/0 and ∞/∞ give the default NaN `0x7ff8000000000000`. A NaN operand comes back quietened.
- **Ties to even:** in every operation, at both normalisations.
- **The division:** divisors just above 2^52 and just below 2^53, where the reciprocal estimate is weakest.
- **Conversions:** `INT64_MIN`; 2^53 + 1 (a tie); `UINT64_MAX`; float subnormals and NaN payloads; doubles at float's range ends; the integer conversions at 2^31, 2^32, 2^63 and 2^64 (saturating), and for |x| < 1.

## Test vectors and validation

- **`test/unit/softdp`, on every board:**
  - 30,000 cases of every operation and conversion, computed by the compiler's own code (the hardware, libgcc, or this layer), with NaNs counted as one value and conversions only in range;
  - the results are folded into a hash whose expected value x86_64's hardware fixes;
  - known answers for the zero signs, the ties, the subnormal and overflow edges, and the division specials.
- **On x86_64:** the layer's C helpers and the two models, called directly against the hardware, case by case (300,000 cases).
- **On armv7e-m and armv8-m.main:** the assembly `dadd`, `dsub` and `dmul` against the models, 400,000 cases. The operands cover every exponent difference up to 80, cancelling, carrying and tying significands, zeros, subnormals, infinities and NaNs, and products near both ends of the range.
- **Mutants caught:**
  - a tie rounded up;
  - the sticky bit dropped from b's long shift;
  - the sticky bit dropped from dmul's underflow shift;
  - the sign of a zero difference of two subnormals.
- **Development runs, not kept:**
  - under qemu, the assembly against the C:
    - 200 million add, subtract and multiply cases;
    - 20 million each concentrated on exponent fields 0–3 and on products that underflow;
    - all 2^32 floats through `f2d`;
    - 100 million cases each of `d2f`, `d2iz`, `d2uiz`, `l2d` and `ul2d`;
  - on the host, 200 million cases of the C helpers against the hardware, and 300 million divisions with divisors near 2^52 and 2^53 and near-exact quotients.

## Results

See PLAN D99, `docs/perf/armv7e-m.md`, `docs/perf/armv8-m.main.md`, `docs/size/armv7e-m.md` and `docs/size/armv8-m.main.md`.

Instructions per operation (armv7e-m and armv8-m.main, both flavours; libgcc → soft-dp; AOR at32 for scale):

| Operation | libgcc | soft-dp | AOR |
|---|---:|---:|---:|
| `dadd` | 63.9 | 55.2 | 46.2 |
| `dsub` | 67.2 | 58.1 | 46.3 |
| `dmul` | 57.6 | 54.0 | 54.9 |
| `ddiv` | 583 | 174 | 82 |
| `dcmp` | 48.9 | 28.9 (43.9 in C) | 20.2 |
| `f2d` | 15.9 | 15.9 | |
| `d2f` | 19.9 | 19.4 | 17.9 |
| `d2i` | 21.9 | 19.9 | |
| `i2d` | 40.9 | 21.6 | 18.9 |
| `l2d` | 48.6 | 30.1 | 29.6 |
| `d2l` | 178 | 44.9 | 25.9 |
| `f2l` | 141 | 27.4 | 20.9 |

- **The double libm:** 15 % fewer instructions on the geometric mean of the 60 cases, up to 47 % (`cbrt`).
  - `sqrt` and `fmod` are within 0.3 %.
  - `fmod`'s latency case is the one regression, +4.3 %. It comes from the benchmark's `x + y * 0.0` dependency, which runs through the zero paths.
- **`strtod`:** up to 30 % (release) and 51 % (speed) fewer instructions.
- **`sscanf("%lf")`:** 13–18 % fewer.
- **Size (release):** `math-mix` −156 bytes and `hello-printf-float` −668 (armv7e-m) or −660 (armv8-m.main). No libgcc floating-point member is linked any more.

## Left open

- **armv7e-m-dp:** the double-precision FPU does the arithmetic. Only the 64-bit conversions are still libgcc's there (the rest of Q24).
- **Division:** `ddiv` (174 instructions) is still well behind AOR's 82. Division is rare in the libm, so assembly for it pays only for code that divides in loops.
- **Comparisons:** now 28.9 instructions (D101), against AOR's 20.2. The rest is the call into the shared ordering; inlining it into the five predicates would cost about 400 bytes.
