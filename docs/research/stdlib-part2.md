# Research: `<stdlib.h>` part 2, `<inttypes.h>`, `<setjmp.h>`, `<signal.h>`

Step: P1.05. Decisions: PLAN D6, D15, D33.

## Sources reviewed

- musl (kraj/musl mirror, <https://github.com/kraj/musl>):
  `src/internal/intscan.c` and `src/stdlib/strtol.c` (one unsigned core with a
  limit, wrappers per type), `src/stdlib/qsort.c` (smoothsort, O(1) memory,
  adaptive), `src/stdlib/bsearch.c`, `src/prng/rand.c` (64-bit LCG, top 31
  bits), `src/setjmp/x86_64/{setjmp,longjmp}.s`, `src/exit/{atexit,exit}.c`
  (a built-in block of 32 handlers, handlers run newest first, then
  `.fini_array`, then stdio), `include/inttypes.h`.
- FreeBSD `lib/libc/stdlib/strtol.c`: cutoff/cutlim overflow detection
  computed once per call, digits consumed past an overflow.
- newlib `libc/search/qsort.c` could not be fetched (empty response); its
  documented design is Bentley–McIlroy quicksort (*Engineering a Sort
  Function*, Software—Practice and Experience 23(11), 1993).
- M. E. O'Neill, *PCG: A Family of Simple Fast Space-Efficient Statistically
  Good Algorithms for Random Number Generation*, HMC-CS-2014-0905, 2014
  (<https://www.pcg-random.org/paper.html>).
- J. W. J. Williams, *Algorithm 232: Heapsort*, CACM 7(6), 1964, and
  R. W. Floyd, *Algorithm 245: Treesort 3*, CACM 7(12), 1964 (bottom-up heap
  construction).
- System V AMD64 psABI §3.2 (callee-saved registers: `rbx`, `rbp`,
  `r12`–`r15`, `rsp`); AAPCS §5.1 (r4–r11, sp; d8–d15 with an FPU).
- The standard text: C23 7.8, 7.13, 7.14, 7.24.

## `strtol` family

| Point | Choice |
|---|---|
| Structure | One core `__libc_strtoint()` parses into `uintmax_t` against a magnitude limit; `strtol`, `strtoll`, `strtoul`, `strtoull`, `strtoimax`, `strtoumax` are wrappers. |
| Overflow | Detected per digit with `value > (limit - digit) / base`; digits are still consumed so `endptr` points past the whole subject sequence (C23 7.24.1.7p5–p8). |
| Signed result | Limit is `MAX` for positive and `MAX + 1` for negative input; the result is the two's-complement negation in `uintmax_t`, clamped to `MIN`/`MAX` with `ERANGE`. |
| Unsigned result | Limit `UMAX` on the magnitude; a leading `-` negates in the return type (`strtoul("-1")` is `ULONG_MAX`). |
| Prefixes | Base 0 or 16: `0x`/`0X`; base 0 or 2: C23 `0b`/`0B`; base 0 with a leading `0`: octal. A prefix not followed by a valid digit leaves `endptr` after the `0`. |
| Bad base | Not 0 or 2–36: undefined; returns 0, `endptr = nptr`, `errno = EINVAL` (as POSIX). |
| No subject | Returns 0 and `endptr = nptr` (whitespace and a sign are not consumed). |
| `atoi`/`atol`/`atoll` | `(int)strtol(nptr, nullptr, 10)` etc. (C23 7.24.1.2). |

## `qsort`: heapsort

| Algorithm | Worst case | Extra memory | Code | Notes |
|---|---|---|---|---|
| Bentley–McIlroy quicksort (newlib, BSD) | O(n²) | O(log n) stack with care | medium | fastest on average |
| Introsort (quicksort + heapsort) | O(n log n) | O(log n) | large | glibc-era designs |
| Smoothsort (musl) | O(n log n), O(n) on sorted input | O(1) | large (Leonardo heaps) | adaptive |
| **Heapsort** | O(n log n) | O(1) | smallest | about 2× slower than quicksort on random data |

Chosen for Phase 1: **heapsort** with Floyd's bottom-up construction, swapping
elements with a word-sized loop when the element size and base are
word-aligned, else bytes. It is the smallest code with a guaranteed bound and
no recursion, which matters more on an MCU stack than average speed. A
quicksort-based speed flavour can come in P3 (D12). `qsort` is not stable,
which the standard allows.

`bsearch`: classic halving on `[low, high)`; the C23 generic macro returns a
`const void *` when `base` points to `const`, with the same `_Generic` trick
as the `<string.h>` search functions.

## `rand`

- The C23 example generator (`next * 1103515245 + 12345`, 15 bits) is weak
  and musl's LCG gives 31 bits but poor low bits in the state.
- **Chosen: PCG32 (XSH-RR)** — a 64-bit LCG state with a permuted 32-bit
  output, of which the top 31 bits are returned; `RAND_MAX` is `0x7fffffff`.
  It is statistically far better than an LCG and costs one 64×64 multiply
  (a libgcc call on armv6-m).
- State is `thread_local` (D6) and starts as if `srand(1)` had been called
  (C23 7.24.2.2p2). `srand(seed)` re-seeds with the PCG initialisation
  sequence.

## Program termination

- `atexit` and `at_quick_exit`: fixed tables of 32 handlers each (the C23
  minimum), no allocation, guarded by the port lock `__rtos_atexit_lock`
  (a new statically allocated seam lock, D31 style).
- `exit`: run the `atexit` handlers newest first (one at a time, so handlers
  that register more are honoured), then the `.fini_array` destructors in
  reverse, then `__rtos_exit`. Stream flushing joins in P1.09.
- `quick_exit`: run the `at_quick_exit` handlers newest first, then `_Exit`.
- `abort`: `raise(SIGABRT)`; if a handler returns, `__rtos_abort` anyway
  (C23 7.24.4.1).
- `getenv` and `system` go through new seam functions
  `__rtos_getenv(name)` and `__rtos_system(command)`. The Linux port searches
  the environment it received; it has no command processor, so
  `system(nullptr)` is 0 and `system(command)` is -1. `port/none` has no
  environment and no command processor.

## Multibyte functions in the "C" locale

The "C" locale is single-byte (`MB_CUR_MAX` is 1) and maps every byte value
to the wide character of the same value. `mblen`/`mbtowc` return 1 (0 for
the null byte); `wctomb` fails with -1 and `EILSEQ` for wide characters above
255; `mbstowcs`/`wcstombs` convert byte by byte. Full multibyte support stays
out of scope (PLAN §1).

## `<inttypes.h>`

The `PRI`/`SCN` macros use the C23 width modifiers instead of per-target
length letters: `PRId32` is `"w32d"`, the least-width ones use `wN` too, the
fastest ones `wfN`, `MAX` uses `j`, `PTR` uses `t` (`intptr_t` and
`ptrdiff_t` are asserted to be the same type). One header then serves every
target with no `#if` (D15), and GCC 14's format checking accepts these for
the matching `<stdint.h>` types on both x86_64 and arm-none-eabi (checked).
`wcstoimax`/`wcstoumax` are wide-character functions, out of scope.

## `<setjmp.h>`

- `jmp_buf` is an array of one structure holding `10 * sizeof(void *) + 64`
  bytes, aligned to 8: room for x86_64's 8 callee-saved registers and for
  Arm's r4–r11, sp, lr plus d8–d15. The same expression serves every target
  without an arch header, and each triple gets one layout (D15).
- x86_64 (`libc/setjmp/x86_64/`): `setjmp` stores `rbx`, `rbp`,
  `r12`–`r15`, the caller's `rsp` and the return address, returns 0;
  `longjmp` reloads them and returns `val`, or 1 when `val` is 0. The Arm
  versions come with P1.06. The floating-point environment is not saved
  (C23 7.13.2.1 does not require it).

## `<signal.h>`

A software implementation with no OS signals (PLAN scope):

- A handler table for the six standard signals, using the Linux numbers
  (`SIGINT` 2, `SIGILL` 4, `SIGABRT` 6, `SIGFPE` 8, `SIGSEGV` 11, `SIGTERM` 15,
  D20).
- `signal` validates the number and returns the previous handler, or
  `SIG_ERR` with `errno = EINVAL`.
- `raise`: `SIG_IGN` does nothing; a handler is reset to `SIG_DFL` and then
  called (C23 7.14.1.1p3 allows the reset); the default action for `SIGABRT`
  is `__rtos_abort`, for the others `_Exit(128 + sig)` (implementation-
  defined, chosen to match how a shell reports a signal).

## Edge cases and test vectors

- `strtol`: `"  -0x1fZ"` base 0 → -31, end at `Z`; `"0x"` → 0, end after
  `0`; `"0b101"` base 0 and 2 → 5; `"0b"` → 0, end after `0`; `"z"` base 36 →
  35; `LONG_MAX`/`LONG_MIN` and one beyond (→ clamp, `ERANGE`, end past all
  digits); `strtoul("-1")` → `ULONG_MAX`; base 1 and 37 → `EINVAL`;
  `"+"`, `"-"`, `""` → 0 with `endptr = nptr`.
- `qsort`: 0, 1, 2 elements; sorted, reversed, all equal; element sizes 1, 3,
  4, 8, 24; random arrays against an insertion sort (libc-test
  `functional/qsort` too).
- `bsearch`: found at both ends, missing below/above/between, empty array.
- `rand`: deterministic sequence after `srand(n)`; the same sequence without
  `srand` as after `srand(1)`; range `0..RAND_MAX`; per-thread state (P1.12).
- `atexit`: 32 registrations succeed, handlers run newest first, a handler
  registering another one.
- `div`: sign of quotient and remainder (truncation toward zero).
- `setjmp`: `longjmp(env, 0)` returns 1; locals declared `volatile` survive;
  jumping out of nested calls.
- `signal`/`raise`: handler runs once and is reset; `SIG_IGN`; invalid
  signals.
