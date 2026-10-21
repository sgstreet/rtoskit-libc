# Research: faster conversions

Steps: P3.03a (integers), P3.03b (floating point). Decision: PLAN D77 (P3.03a).

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
