# Research: formatted-output engine (integer core)

Step: P1.02. Float conversions follow in P1.08 (`docs/research/` note of
that step). Decisions: PLAN D6 (one function per file), D13 (replaceable
float conversion).

## Sources reviewed

- musl `src/stdio/vfprintf.c` (read from the kraj/musl mirror,
  <https://github.com/kraj/musl/blob/master/src/stdio/vfprintf.c>, because
  git.musl-libc.org did not resolve from this session). A flag bitmask
  indexed by `c - ' '`, a table-driven state machine for the length
  modifiers, `pad()` that writes from a 256-byte stack buffer, and digit
  generation backwards into a buffer (`fmt_x`, `fmt_o`, `fmt_u`; `fmt_u`
  runs a 64-bit loop while the value exceeds `ULONG_MAX`, then a `long`
  loop). Positional arguments (`%n$`, POSIX) need a two-pass scan with an
  `NL_ARGMAX` argument table. Overflow of the `int` result sets
  `EOVERFLOW`. This mirror has no C23 `%b`/`%wN`.
- picolibc tinystdio `libc/stdio/vfprintf.c`
  (<https://github.com/picolibc/picolibc/blob/main/libc/stdio/vfprintf.c>).
  Build-time variants (minimal, integer, long long, float, double) choose
  features with `_NEED_*` macros. The integer conversion buffer is a union
  sized for the widest value in the smallest base. Float arguments are
  consumed even in the integer-only variant. `%b` and `%wN`/`%wfN` are
  supported, with the width mapped to a size flag. Positional arguments
  rescan the format to skip to the argument.
- eyalroz/printf (fork of mpaland/printf),
  <https://github.com/eyalroz/printf>. An "output gadget" (a callback plus
  state) separates formatting from the destination; `snprintf` with a
  `NULL` buffer only counts. `%b` with `#` gives `0b`. Compile-time options
  drop `%n`, `long long` and floats.
- Linux `lib/vsprintf.c`
  (<https://github.com/torvalds/linux/blob/master/lib/vsprintf.c>).
  `format_decode()` returns one token at a time into a compact
  `struct printf_spec`. `number()` composes sign, prefix, precision zeros
  and width padding; zero padding goes after the sign and prefix.
  `put_dec()` writes two digits at a time from a digit-pair table and
  avoids 64-bit division on 32-bit machines. The output pointer advances
  past the end of the buffer while stores are guarded, so the return value
  is the full length.
- The standard text: C23 7.23.6.1 (`fprintf`), 7.23.6.5/6/12/13 (`snprintf`,
  `sprintf`, `vsnprintf`, `vsprintf`).

## Trade-offs

| Choice | Options | Chosen |
|---|---|---|
| Destination | per-character callback; chunked callback; buffer only | Chunked `write(sink, s, n)` callback (`struct __libc_format_sink`). One engine serves `snprintf` now and `FILE` streams in P1.09. Literal runs and digit strings are written in one call. |
| Counting | count in sink; count in engine | The engine counts in `size_t` and checks each field against `INT_MAX` *before* writing it, so a huge width fails fast with `EOVERFLOW` instead of writing gigabytes to a counting sink. |
| Length modifiers | map to a bit width; keep the modifier | Keep the modifier (an enum). `va_arg` then uses the exact type (`int32_t`, `int_fast16_t`, `size_t`, ...), which is correct whatever typedef the target uses (`int32_t` is `long` on arm-none-eabi). `%n` stores through the exact pointer type. |
| Digit generation | divide per digit; digit pairs; table | Backwards into a 64-byte buffer (enough for 64 binary digits). Base 2/8/16 use shifts. Base 10 uses 64-bit division only while the value exceeds `UINT32_MAX`, then a 32-bit loop (as musl does with `long`), so 32-bit Arm calls `__aeabi_uldivmod` at most twice for any 64-bit value. Digit pairs are a P3.03 speed-flavour option. |
| Padding | stack buffer; constant strings | Constant 16-byte runs of spaces and zeros, written repeatedly. No stack buffer. |
| Positional `%n$` | support; reject | Not supported: it is POSIX, not ISO C (PLAN §1 scope). |
| Floats | in the engine; separate symbol | The engine fetches the argument (`double`, or `long double` with `L`) and calls `__libc_format_float()`, one internal symbol that P1.08 implements and that the P2 no-float flavour (D13) can replace. Until P1.08 a stub fails with `EINVAL`. |

## Chosen behaviour (implementation-defined or undefined cases)

- `%p`: `0x` followed by lowercase hexadecimal digits, also for a null
  pointer (`0x0`). Width and `-` apply.
- `%s` with a null pointer (undefined): prints `(null)`.
- `%lc`/`%ls`: wide characters are out of scope (PLAN §1). The call fails
  with a negative result and `EILSEQ`.
- An unsupported `wN`/`wfN` width (anything but 8, 16, 32, 64; or a leading
  zero) makes the call return a negative value, as C23 7.23.6.1p15 allows,
  with `errno = EINVAL`. The same applies to an invalid conversion
  specifier, `H`/`D`/`DD` (no decimal floating types), and a `*` width of
  `INT_MIN`.
- A result longer than `INT_MAX` returns a negative value with
  `errno = EOVERFLOW` (both error numbers are added to `<errno.h>` with Linux
  values, D20).
- `B` is implemented (C23 7.23.6.1p14 encourages it); `#` gives `0B`.
- `vsnprintf(s, 0, ...)` never touches `s` (it may be null). Otherwise at
  most `n - 1` characters are stored and a null character always follows.
  The return value is the untruncated length. `vsprintf` is `vsnprintf` with
  an unlimited count; the buffer sink counts the room down and never forms
  an end pointer, so `SIZE_MAX` is safe.

## Edge cases and test vectors (test/unit/printf, libc-test)

- `%.0d` of 0 → `""`; `%#.0o` of 0 → `"0"`; `%#o` of 8 → `"010"`;
  `%#.3o` of 8 → `"010"` (precision already supplies the zero).
- `%#x` of 0 → `"0"` (no prefix); `%#x` of 255 → `"0xff"`; `%#b` of 5 →
  `"0b101"`; `%#B` of 5 → `"0B101"`.
- `%05d` of -42 → `"-0042"`; `%-05d` of -42 → `"-42  "` (0 ignored with
  `-`); `%05.3d` of 7 → `"  007"` (0 ignored with a precision).
- `%+d`/`% d` of 5 → `"+5"`/`" 5"`; `%+ d` → `"+5"` (space ignored).
- `%*d` with width -4 → left-justified; `%.*d` with precision -1 → as if
  omitted.
- `%hhd` of 300 → `"44"`; `%hd` of 65537 → `"1"`; `%hhu` of -1 → `"255"`.
- `%jd` of `INTMAX_MIN` → `"-9223372036854775808"`; `%llo`, `%llx`, `%llb`
  of `UINT64_MAX`.
- `%w8d`, `%w16x`, `%w32u`, `%w64d`, `%wf8d`..`%wf64d`; `%w7d`, `%w08d`,
  `%w128d` → negative result.
- `%.3s` of `"abcdef"` → `"abc"`, and the source need not be null
  terminated when the precision is within the storage.
- `%n` with every length modifier (libc-test `regression/printf-fmt-n`).
- `snprintf(NULL, 0, "%d", 12345)` → 5; truncation keeps a null character;
  `snprintf` with `n == 1` stores only the null character.
- A width above `INT_MAX` or a total above `INT_MAX` → negative,
  `EOVERFLOW`.
