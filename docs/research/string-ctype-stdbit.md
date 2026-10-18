# Research: `<string.h>`, `<ctype.h>`, `<assert.h>`, `<stdbit.h>`, `<stdckdint.h>`

Step: P1.03. Generic C only; per-arch speed versions are P3.02.
Decisions: PLAN D6 (one function per file), D8 (freestanding build), D30.

## Sources reviewed

- musl `src/string/{strlen,memchr,strchrnul,memcpy,memmove,memset,memcmp,strcmp,strspn,strstr}.c`
  (kraj/musl mirror, <https://github.com/kraj/musl/tree/master/src/string>)
  and `src/ctype/isalpha.c`.
- glibc generic word helpers `sysdeps/generic/string-fza.h`, `string-fzb.h`
  and `string/strlen.c` (<https://github.com/bminor/glibc>).
- FreeBSD `lib/libc/string/strlen.c`
  (<https://github.com/freebsd/freebsd-src/blob/main/lib/libc/string/strlen.c>).
- Sean Anderson, *Bit Twiddling Hacks*, "Determine if a word has a zero
  byte" (<https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord>),
  which glibc cites.
- M. Crochemore and D. Perrin, *Two-way string-matching*, J. ACM 38(3),
  1991; the algorithm summary at
  <https://en.wikipedia.org/wiki/Two-way_string-matching_algorithm>.
- GCC 14 manual, "Other Built-in Functions": `__builtin_stdc_*`
  (type-generic `<stdbit.h>` operations), `__builtin_{add,sub,mul}_overflow`.
- The standard text: C23 7.2, 7.4, 7.18, 7.20, 7.24.9.1, 7.26.

The newlib and picolibc string sources were not reachable from this session
(the raw GitHub fetches returned empty files); their designs are not part of
this note.

## Word-at-a-time scanning

### Technique

With `w` a word of `N` bytes, `ONES` = `0x01` repeated and `HIGHS` = `0x80`
repeated:

- `(w - ONES) & ~w & HIGHS` is non-zero iff some byte of `w` is zero. The
  lowest set `0x80` bit marks the *first* zero byte on a little-endian
  machine; higher bytes may be false positives (a borrow from a zero byte
  turns a `0x01` above it into `0x80`), so only "any" and "first" are exact.
- Searching for byte `c`: apply the same test to `w ^ (ONES * c)`.
- All three reviewed libcs read whole *aligned* words. An aligned word never
  crosses a page (or MPU region) boundary, so reading bytes past the
  terminator inside the same word cannot fault. It is outside the C object
  model, which is why musl and glibc read through a `may_alias` type.

### Choices

| Topic | Options | Chosen |
|---|---|---|
| Word type | `size_t`, `unsigned long`, `uintptr_t` | `uintptr_t`: 32 bits on Arm, 64 on x86_64. |
| Aliasing-safe load | `may_alias` typedef (musl, glibc); `memcpy` | `__builtin_memcpy` of an aligned word (`__builtin_assume_aligned`) in `libc/internal/word.h`. Explicit builtins are expanded even under `-fno-builtin`, compile to one load, and need no new typedef. |
| Finding the byte in the word | count trailing zeros of the mask; byte loop | Byte loop over the (at most `N`) bytes of the matching word: no endianness dependence, and `ctz` is a libgcc call on armv6-m. |
| Which functions | musl: `strlen`, `memchr`, `strchrnul`, `memcpy`, `memset`, `memmove` | `strlen`, `memchr`, `strchr`/`strchrnul` core, `memset`, and the co-aligned paths of `memcpy`/`memmove`. The rest are byte loops or built on those. |
| `memcpy`/`memmove` misaligned | shift-and-merge (musl `memcpy`); bytes | Word copy only when source and destination are co-aligned; otherwise bytes. Shift-merge and unaligned access (v7-M) belong to P3.02. |
| `memcmp`, `strcmp` | word compares | Byte loops: the first difference must be located byte by byte anyway, and P3.02 replaces them with `UADD8`/`SEL` code. |

## `strstr`: Two-Way

- Naive search is O(n·m). Two-Way (musl and glibc use it) is O(n + m) time
  and O(1) space, which matters for untrusted input.
- Critical factorization: compute the maximal suffix of the needle under `<`
  and under `>`, take the one that starts later (`ℓ`), with period `p`.
- If `x[0..ℓ)` reappears at `x[p..p+ℓ)` (and `ℓ + p ≤ m`), the needle is
  periodic: after a full right-half match and a left-half mismatch, shift
  by `p` and remember that `m - p` bytes already match. Otherwise shift by
  `max(ℓ, m - ℓ) + 1` with no memory.
- Right half is compared left to right from `ℓ`; a mismatch at `i` shifts by
  `i - ℓ + 1`. Then the left half is compared right to left.
- The haystack length is found with `strlen` after locating the first
  needle character with `strchr` (musl grows the known length lazily; one
  extra linear pass keeps ours simpler).
- Needles of length 0 return the haystack; length 1 is `strchr`.

## `strspn`, `strcspn`, `strpbrk`

A 256-bit set (`CHAR_BIT * 32` bytes on the stack) built from the accept or
reject string, then one pass over `s` (musl). Single-character reject sets
use `strchrnul` directly. `strpbrk` is `s + strcspn(s, accept)`.

## Other `<string.h>` decisions

- **Generic functions (C23 7.26.5.1):** `memchr`, `strchr`, `strpbrk`,
  `strrchr`, `strstr` are also macros that return a `const`-qualified pointer
  when the first argument points to `const`, using `_Generic` on the first
  argument. The controlling expression is not evaluated, so arguments are
  evaluated once. Taking the address (`memchr` without parentheses) still
  names the function.
- **`memset_explicit`:** `memset` followed by an empty `asm volatile` that
  takes the pointer and clobbers memory, so the stores cannot be removed as
  dead.
- **`strtok`:** the save pointer is a `thread_local` in `strtok.c` (D6).
- **`strcoll`/`strxfrm`:** the "C" locale only (until P1.11): `strcoll` is
  `strcmp`; `strxfrm` copies and returns `strlen(s2)`, copying nothing when
  `n` is too small.
- **`strerror`:** a table indexed by error number for the numbers
  `<errno.h>` defines, otherwise `"Unknown error"`. Messages are
  read-only string literals; the full table grows with `<errno.h>`
  (P1.11).
- **`memalignment`** (`<stdlib.h>`): the lowest set bit of the address,
  `a & -a`, which is 0 for a null pointer.

## `<ctype.h>`: table-free

- Options: a 256-entry (or 384 with `EOF`) classification table (newlib,
  BSD, glibc) versus range checks (musl: `(unsigned)c|32)-'a' < 26`).
- Only the "C" locale is in scope until P1.11, and each function is one or
  two unsigned comparisons. Table-free costs no data, so it is chosen.
  Values outside `unsigned char` and `EOF` are undefined behaviour; `EOF`
  (negative) fails every check because of the unsigned arithmetic.
- Functions only, no macros, so P1.11 can add locale support behind the same
  symbols.

## `<assert.h>`

- C23 7.2: `assert(...)` is variadic so commas in compound literals work;
  the header redefines `assert` on every inclusion according to `NDEBUG`,
  so the macro part sits outside the include guard (the standard requires the
  conditional definition, CLAUDE.md allowed use 3).
- Failure calls `__libc_assert_fail(expression, file, line, function)`,
  which formats `file:line: function: Assertion 'expression' failed.` with
  `snprintf`, writes it with `fputs(stderr)` (no `fprintf` before P1.09)
  and calls `abort`.

## `<stdbit.h>` and `<stdckdint.h>`

- GCC 14 provides type-generic `__builtin_stdc_*` for all 14 operations,
  including bit-precise types, and `__builtin_{add,sub,mul}_overflow` with the
  exact `ckd_*` semantics (infinite-precision result, wrapped store, `true`
  on overflow).
- The type-generic macros expand to those builtins. The 70 typed functions
  (14 operations × 5 types) each apply the generic macro to their typed
  argument, one per file (generated once by a script kept out of the tree;
  each file is ordinary source).
- Endianness macros come from the compiler: `__ORDER_LITTLE_ENDIAN__`,
  `__ORDER_BIG_ENDIAN__`, `__BYTE_ORDER__`, so no `#if`.
- Clang 19 has `__builtin_clzg`/`ctzg`/`popcountg` but not the
  `__builtin_stdc_*` family; Clang is not enabled (D9), so no fallback is
  written now (Q11).

## Edge cases and test vectors

- `strlen`, `strchr`, `memchr` at every alignment 0..15 and length 0..64,
  with the target byte at every position, bytes `0x80`/`0xff` around it
  (false-positive borrow case), and `c` passed as an `int` outside
  `unsigned char` (it is converted).
- `strchr(s, '\0')` returns the terminator; `strrchr` too.
- `memmove` overlapping both directions at every offset; `memcpy` with every
  co-alignment.
- `memccpy` stops after `c`, returns the next destination byte, or null if
  `c` is absent; `c` converted to `unsigned char`.
- `strncpy` pads with zeros and does not terminate when `n <= strlen`;
  `strncat` always terminates.
- `strtok` with leading, repeated and trailing delimiters, and an all-
  delimiter string (returns null).
- `strstr`: empty needle; needle longer than haystack; periodic needles
  (`"aaab"`, `"abab"`), and an exhaustive comparison with a naive search over
  all strings of length ≤ 10 on a two-letter alphabet.
- `ctype`: all 256 values plus `EOF` against the C-locale definitions.
- `<stdbit.h>`: 0, 1, max, powers of two and their neighbours for every type;
  `bit_ceil` returns 0 when the result does not fit.
- `ckd_*`: mixed signedness and widths, `INT_MIN - 1`, `UINT_MAX + 1`,
  products near the limits, results wrapped on overflow.
