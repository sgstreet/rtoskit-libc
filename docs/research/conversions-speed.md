# Research: faster conversions

Steps: P3.03a (integers), P3.03b (floating point). Decisions: PLAN D77 (P3.03a), D78 (P3.03b).

## P3.03a: integer conversions

### Where the time went (P3.01 baseline)

| Case | armv6-m | armv7e-m | x86-64 |
|---|---:|---:|---:|
| `strtol("42")` | 914 | 949 | 162 |
| `strtol("-2147483647")` | 1501 | 1213 | 342 |
| `atoi("-123456")` | 1211 | 1081 | 258 |
| `strtoll("9223372036854775807")` | 2350 | 1496 | 538 |
| `snprintf("%d")` | 1636 | 468 | 525 |
| `snprintf("%lld")` | 11025 | 7636 | 790 |
| `snprintf(literal, 34 chars)` | 555 | 512 | 686 |

- **`__libc_strtoint`** (D33) computes the overflow cutoff once per call as
  `__libc_udivmod64(limit, base)`, a 64-by-32 division in software that
  costs about 800 instructions on the 32-bit cores. It also does every
  `value * base + digit` in `uintmax_t`, which on Armv6-M is a
  `__aeabi_lmul` call per digit.
- **`__libc_format_digits`** (D54) divides a 64-bit value by 10 with
  `__libc_udivmod64` for every digit above 32 bits. That is 10 such
  divisions for a 19-digit `%lld`. Below 32 bits it uses `x / 10`, which is
  one `UMULL` on v7-M and x86-64, but a `__aeabi_uidivmod` call on Armv6-M,
  which has neither a divider nor a multiply-high.
- **The buffer sink** (`snprintf`) copies every piece byte by byte, so the
  literal text alone costs about 6 instructions a character.

### Sources reviewed

- **Hacker's Delight** (Warren, 2nd ed.) ch. 10, *Integer division by
  constants*: reciprocal multiplication. `x / 100 == (x * 0x51eb851f) >> 37`
  for every 32-bit x, which is the constant GCC uses.
- **The digit-pair technique** (two digits per division by 100, from a
  200-byte table or computed). It is used by fmtlib, by Andrei
  Alexandrescu's "Three optimization tips for C++" talk and by the
  jeaiii/itoa line of work. Shown by them, not copied here.
- **C23 7.24.1.7** (`strtol` and relatives): the subject sequence, the
  value, the end pointer, and `ERANGE` with `LONG_MAX`/`LONG_MIN`/
  `ULONG_MAX`. Then **7.20** (`<stdckdint.h>`): `ckd_mul` and `ckd_add`
  report wrap-around, and GCC implements them with the carry or overflow
  flag (one `UMULL` and a test on v7-M).
- **musl and picolibc `strtol`** compare against a precomputed cutoff,
  as D33 does; **glibc** uses `__builtin_mul_overflow` in parts of its
  `strtol` family.

### Design: per-module speed files (D12)

The size budget allows the size flavour no growth at all. The size
flavour's printf images link the generic files, and the faster versions
are larger. So the speed versions live in a per-module `speed/` directory:

- `libc/stdio/speed`, `libc/stdlib/speed`;
- `subdir.mk` drops the generic file of the same name when `LIBC_FLAVOUR`
  is `speed`;
- `libc/libc.mk` archives the speed objects in that flavour only;
- `tools/makefiles/speed.mk` now *exports* `LIBC_FLAVOUR`, because
  `libc.mk` expands its object list before `common.mk` is read.

The flavour archives replace members by file name, so
`libc-minimal-printf.a` of the speed flavour still takes its own
`__libc_format_digits`.

### Kernels

**`__libc_strtoint`** (speed): three phases, and no division at all.

1. While the value is below 2^32 / 36, another digit of any base cannot
   overflow 32 bits, so it accumulates in `uint32_t` with no check. That
   covers every input of up to about 8 digits.
2. A limit that fits 32 bits (`long` on a 32-bit core) continues with
   `ckd_mul`/`ckd_add` on `uint32_t`.
   - A first version divided the limit by the base (one 32-bit division):
     cheaper on v7-M, which has `UDIV` (282 against 305), but 641 against
     422 on Armv6-M, where it is a helper call.
3. A wider limit continues in `uintmax_t`, unchecked while the value is
   below 2^64 / 36. Only the last one or two digits of a 19-digit number
   take the checked operations.

The limit is compared once at the end.

**`__libc_format_digits`** (speed):
- A decimal value above 32 bits is cut into chunks of 10^9 with at most
  two 64-bit divisions, where the size flavour does one per digit. Each
  chunk is written two digits per 32-bit division by 100, the pair split
  as `(r * 205) >> 11` (exact for r < 1029) and `r - 10 * tens`.
- The power-of-two bases shift a 32-bit copy once the value fits one. The
  size flavour shifts `uintmax_t`, a `__aeabi_llsr` call per digit on
  Armv6-M.
- Everything else, notably 32-bit decimals, is the size flavour's code,
  unchanged. Measured:
  - Pairs for 32-bit values gained nothing where `x / 10` is one `UMULL`
    (v7-M, x86-64): +11 instructions for `%d`.
  - A 200-byte pair table, or the reciprocal multiplication, put
    `hello-printf-int-nofloat` 90 bytes over the speed budget on
    armv7e-m-dp and armv8-m.main, and pulled `__aeabi_lmul` into it on
    armv6s-m.
  - Inlining the pair code made the hex loop and small decimals 6–52
    instructions slower through worse register allocation. So the pair
    code sits in a `noinline` function that the common path never pays
    for.

**`__libc_format_buffer_write`** (speed):
- Pieces of 16 bytes or more go through `memcpy` (the speed flavour's
  block copy); shorter ones are copied in place.
- The buffer fields are updated first, so the `memcpy` is a tail call and
  the short path needs no stack frame.
- An unconditional `memcpy` cost 5–36 instructions per call on short
  pieces (`%c`, `%e`).

**`-O2` for the engines** (`LIBC_SPEED_CFLAGS`):
- The printf engine at `-O2` saved 60–130 instructions per call, but put
  every printf image 12–34 % over the speed budget, so it stays at `-Os`.
- The scanf engine is in no `test/size` image, so it is compiled at `-O2`
  in the speed flavour.

### Results (speed flavour, instructions per call)

| Case | armv6-m | armv7e-m | x86-64 |
|---|---|---|---|
| `strtol("42")` | 914 → 170 | 949 → 135 | 162 → 150 |
| `strtol("-2147483647")` | 1501 → 422 | 1213 → 305 | 342 → 312 |
| `strtoll` 19 digits | 2350 → 1060 | 1496 → 516 | 538 → 495 |
| `atoi` | 1211 → 277 | 1081 → 207 | 258 → 230 |
| `snprintf("%lld")` | 11025 → 3702 | 7636 → 1353 | 790 → 624 |
| `snprintf("%x")` | 629 → 514 | 511 → 450 | 513 → 506 |
| `snprintf` literal | 555 → 354 | 512 → 308 | 686 → 479 |
| `sscanf` mix | 3294 → 2756 | 2081 → 1624 | 2245 → 1672 |

- No case is slower than the P3.02 speed baseline beyond the gate, and the
  speed budget holds.
- **Not done:** 32-bit `%d` on Armv6-M (1636 → 1622) still calls the
  division helper per digit. A shift-and-add division by 10 (Hacker's
  Delight 10-8) would help there and only there; it needs an Armv6-M
  variant of the speed file, left for P3.06.

### Tests

`test/unit/conversions` runs on every board and flavour.

**`strto*`:** `strtol`, `strtoul`, `strtoll` and `strtoull`
- in every base from 2 to 36, at ±2 around 0, 2^32 / 36, `INT32_MAX`,
  `UINT32_MAX`, 2^64 / 36, `INT64_MAX` and `UINT64_MAX`, and at 64
  pseudo-random values of every length;
- each with signs, white space, trailing text, an extra digit and the
  `0x` and base-0 forms;
- checking the value, `errno` and the end pointer against a reference
  parse with checked arithmetic.

**`snprintf`:** `%llu`, `%lld`, `%llx`, `%llo` and `%u|%x` over the 10^9
and 2^32 boundaries, `UINT64_MAX`, `INT64_MIN` and 200 pseudo-random
values.

**Mutants:** all 11 are killed.
- On armv7e-m, every one fails the test:
  - the 32-bit and 64-bit unchecked limits;
  - the two final limit tests;
  - the checked add;
  - the divisor by 100, the pair split, the chunk size;
  - the 64-bit hex threshold.
- Removing the chunk's zero padding turns its loop into an endless one,
  which `run-unit`'s timeout catches.
- The 32-bit-path mutants pass on x86-64 only because `long` is 64 bits
  there, where that path never runs.

## P3.03b: decimal to binary floating point

### Where the time went (P3.03a speed baseline, instructions per call)

| Case | armv6-m | armv7e-m | armv7e-m-dp | x86-64 |
|---|---:|---:|---:|---:|
| `strtod("3.14")` | 3352 | 1962 | 1151 | 406 |
| `strtod("2.7182818284590452")` | 23288 | 4472 | 4275 | 4083 |
| `strtod("6.02214076e+300")` | 689247 | 170760 | 170534 | 210792 |
| `strtod("2.2250738585072014e-308")` | 1839969 | 86186 | 85988 | 125365 |
| `strtof("1.17549435e-38")` | 77753 | 7360 | 7363 | 8812 |

- **Simple Decimal Conversion** (D38) takes every input that Clinger's fast
  path refuses: more than 2^53 in the significand (most 17-digit inputs),
  an exponent beyond ±22, or a directed rounding mode. It shifts an
  800-digit decimal by at most 28 bits at a time, so its cost grows with
  the exponent: 1.8 million instructions for `DBL_MIN` on Armv6-M.
- **The parser** calls `isdigit` for every character, and the fast path
  divides 2^53 by ten with `__libc_udivmod64` on every call.

### Sources reviewed

- W. D. Clinger, *How to Read Floating Point Numbers Accurately*, PLDI
  1990: the exact fast path, and the "extended precision" idea of
  approximating w · 10^q with a bounded error and checking the bound.
- D. Lemire, *Number Parsing at a Gigabyte per Second*, Software: Practice
  and Experience 51(8), 2021 (<https://arxiv.org/abs/2101.11408>), and N.
  Tao, *The Eisel-Lemire ParseNumberF64 Algorithm*
  (<https://nigeltao.github.io/blog/2020/eisel-lemire.html>): w normalised to
  64 bits times a 128-bit truncated 5^q from a 651-entry table (about
  10 KiB), a test of the product's low bits for an ambiguous rounding, a
  retry with w + 1 when digits were dropped, and a fallback.
- N. Mushtak and D. Lemire, *Fast Number Parsing Without Fallback*,
  Software: Practice and Experience 53(6), 2023
  (<https://arxiv.org/abs/2212.06644>): with the full 128-bit table, inputs
  of up to 19 digits never need the fallback.
- U. Adams, *Ryū: fast float-to-string conversion*, PLDI 2018, and its
  "small table" mode (`d2s_small_table.h`, also in picolibc's
  `ryu_table.c`): 5^i rebuilt from a 26-entry table of exact small powers
  and 13 entries of large ones, with one multiplication and a correction.
- Abseil `from_chars` (the Eisel–Lemire change,
  <https://pigweed.googlesource.com/third_party/github/abseil/abseil-cpp/+/dc6109e0c8b2f6ba0fde389050ff6de6c9a082e7%5E%21>),
  Wuffs (the binary exponent from a linear formula instead of a table,
  <https://skia.googlesource.com/external/github.com/google/wuffs/+/b6d855210c872092b8c3119715ee9712fdeb44d0%5E%21/>),
  and the `compact` feature of the Rust crate lexical-parse-float
  (<https://docs.rs/lexical-parse-float/>), which trades table size for speed
  the same way.

### Design

A full Eisel–Lemire table (10 KiB) would be larger than the whole
`strtod` path, so the speed flavour rebuilds 5^q at run time from two
small tables, Ryū-fashion, and keeps an explicit error bound instead of
proving the table exact:

- **Tables (624 bytes):** 5^r for r in [0, 27], exact in 64 bits, and
  5^(28j) for j in [-13, 11], normalised and truncated to 128 bits. q =
  28j + r covers [-364, 335]; outside it every input of up to 19 digits
  overflows or is far below the smallest subnormal, and Simple Decimal
  Conversion handles it at once.
- **Binary exponent:** floor(28j · log2 5) is 65j for j >= 0 and 65j - 1 for
  j < 0 over that range (checked with exact arithmetic), so the scale is
  127 - 65j + (j < 0) and needs no table.
- **Products:** c · 5^r (128 × 64, keeping the top 128 bits), then that
  times w normalised (the same again), each normalised by at most one more
  shift. On 32-bit cores each 64 × 64 product is four 32 × 32 → 64
  multiplications (`UMULL` on v7-M, `__aeabi_lmul` on Armv6-M).
- **Error:** c is truncated (below by less than 1 unit), each product drops
  its low 64 bits (less than 1), and the factor's error is multiplied by w
  normalised (less than 4 units after the first shift). The result is below
  the true value by less than 10 units of its last bit. For j = 0 the
  factors are exact and so is the product.
- **Dropped digits:** past 19 digits w is truncated, so the value lies in
  [w, w + 1) · 10^q. With 19 digits w >= 10^18 > 2^59, which adds less than
  (high >> 59) + 1 units of the high word.
- **The check:** the rounding of the value is known when the interval
  [approximation, approximation + error) neither starts on nor reaches a
  half-ulp boundary at the format's precision. Then the high word goes to
  `__libc_float_pack` with the sticky bit set, which rounds in any
  direction, subnormals included: the boundaries of a coarser (subnormal)
  rounding are a subset of the finer ones. The start test is needed only
  because digits past the 19th may all be zero (`"115292150460684697600000"`
  is exact).
- **Order:** Clinger's fast path still goes first. Once its guard no longer
  divides (`significand * 10 <= limit` replaces the cutoff) and the parser
  tests digits in line, one floating-point operation beats sixteen 32-bit
  multiplications on every board, even in software floating point:
  - `strtod("3.14")` on x86-64: 295 instructions Clinger-first, 626
    middle-path-first;
  - `strtof("3.14")` on Armv6-M: 1008 against 1905.
  The middle path takes what Clinger refuses: wider significands, larger
  exponents and directed rounding. Simple Decimal Conversion is left with
  ties and near-ties, more than 19 digits close to a boundary, and the far
  ends of the exponent range.
- **Not done:**
  - A per-family native multiplication (`unsigned __int128` on x86-64)
    would help only the middle path there.
  - A full table would remove the fallback for up to 19 digits (Mushtak and
    Lemire), but costs 10 KiB to spare the rare inputs that still fall
    back.

### Results (speed flavour, instructions per call)

| Case | armv6-m | armv7e-m | armv7e-m-dp | x86-64 |
|---|---|---|---|---|
| `strtod("3.14")` | 3352 → 2459 | 1962 → 1137 | 1151 → 326 | 406 → 295 |
| `strtod("12345")` | 2909 → 2005 | 1497 → 648 | 1234 → 385 | 490 → 343 |
| `strtod` 17 digits | 23288 → 3799 | 4472 → 1377 | 4275 → 1180 | 4083 → 1049 |
| `strtod` 51 digits | 46683 → 5026 | 7433 → 2399 | 7236 → 2202 | 8603 → 1974 |
| `strtod("6.02214076e+300")` | 689247 → 4087 | 170760 → 1314 | 170534 → 1088 | 210792 → 949 |
| `strtod` `DBL_MIN` | 1839969 → 3853 | 86186 → 1439 | 85988 → 1241 | 125365 → 1106 |
| `strtof("1.17549435e-38")` | 77753 → 2433 | 7360 → 939 | 7363 → 942 | 8812 → 866 |
| `sscanf("%lf")` | 5088 → 4076 | 3378 → 2390 | 2752 → 1764 | 2169 → 1812 |

- No case is slower beyond the gate. The hexadecimal parser is untouched
  (Armv6-M +0.7 %, within noise).
- The speed flavour's `__libc_strtofloat` grows from about 1.1 KiB to
  2.4 KiB on every target (624 bytes of tables); the speed budget holds,
  since no `test/size` image parses floating point.

### Tests

`test/unit/strtod` runs on every board and flavour, so it checks the
generic and the speed paths alike.

- **Short inputs (new):** up to 24 random digits times a random power of
  ten across each format's range, and integers one bit wider than the
  precision (ties when odd) times a small power of two. Every direction's
  result is checked against the input by exact decimal comparison with the
  result, its successor and their midpoint, which the test expands itself.
- **Fixed cases (new):**
  - `2^60 · 10^5` written with 24 digits, the dropped ones all zero or not;
  - inputs on which a lost carry or sticky bit in the middle path rounds
    wrongly;
  - a 19-digit significand whose tenfold wraps to 4 in 64 bits.
  The middle-path inputs were found with a Python model of it, which
  agreed with exact rounding on two runs of 58,000 random decisions (under
  0.1 % fell back).
- **Unchanged:** the P1.07 cases and the random values, midpoints and
  nudged midpoints in every direction.

**Mutants:** 10 of 12 fail the test on x86-64 and armv7e-m:
- the error bound and both boundary tests;
- the dropped-digit margin and the exactness flag;
- the carries of both products;
- the scale's j < 0 term;
- the sticky bit from the low word;
- Clinger's wrap guard.

Two survive, both equivalent:
- dropping the bit shifted into the low word stays inside the error bound;
- a mask one bit narrower only sends more inputs to the fallback.
