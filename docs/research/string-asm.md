# Research: hand-written string kernels for size

Steps: P2.06a (Thumb-1), P2.06b (Thumb-2). Decision: PLAN D69. It builds
on D21 (the word-at-a-time C kernels, P1.03) and D12/Q3 (size and speed
flavours).

## Sources reviewed

- Arm, *Armv6-M Architecture Reference Manual* (DDI 0419) and *Armv7-M*
  (DDI 0403), the Thumb instruction sets:
  - Thumb-1 is 16-bit only. Its ALU works on r0–r7, flags are set by almost
    every data-processing instruction, and there is no IT, CBZ or
    post-indexed load.
  - ldm/stm with writeback, register-offset ldr/str/ldrb/strb and uxtb
    exist.
  - Armv6-M faults on any unaligned word access. Armv7-M allows unaligned
    ldr/str but never unaligned ldm/stm.
  - Thumb-2 adds 32-bit encodings, IT blocks, CBZ/CBNZ and post-indexed
    loads.
- Arm, *Procedure Call Standard for the Arm Architecture* (AAPCS32):
  - r0–r3 and r12 are free;
  - r4–r11 must be preserved;
  - a Thumb-1 b reaches only ±2 KB, and the linker inserts no veneer for
    it.
- ARM optimized-routines `string/arm` and newlib's `libc/machine/arm`
  (descriptions only): they are tuned for speed, with large unrolled loops,
  ldrd and lane tricks, at hundreds of bytes per function. picolibc's
  Thumb-1 builds use the generic C.
- Our own C kernels (D21): the same phase check, alignment head, word body
  and byte tail. The compiler's `-Os` output is the baseline to beat.

## Choice

The word-at-a-time algorithms stay; only their encoding is hand-written.
- A byte-only kernel would be smaller still: a `strlen` in 16 bytes, a
  `memcpy` in about 20. But it would be 3–4 times slower on long strings.
- D12/Q3 leave the speed–size split to separate flavours, and the speed
  flavour does not exist before Phase 3. Losing the P1.03 speed in the
  only library there is was not acceptable for these savings.

## Thumb-1 kernels (`libc/string/thumb1`, armv6-m and armv6s-m)

| Function | C, `-Os` (bytes) | Thumb-1 (bytes) | Notes |
|---|---:|---:|---|
| `memcpy` | 96 | 60 | phase check; head bytes; `ldmia/stmia` words; tail bytes forwards with a negative index from the end |
| `memmove` | 176 | 60 (with its literal) | forward case tail-calls `memcpy` through a register; backward case with indexed loads (bytes to an aligned end, words, bytes); equal pointers return at once |
| `memset` | 72 | 48 | `uxtb` and two shift-or steps replicate the byte; `stmia` words; tail with an index counting down |
| `strlen` | 60 | 48 | bytes to alignment, `ldmia` words tested with (w − 0x01010101) & ~w & 0x80808080 (exact for the first zero byte), then bytes from that word's start |
| `strcmp` | 22 | 20 | the byte loop; the difference of zero-extended bytes |
| `setjmp`/`longjmp` | 22 / 30 | (unchanged) | already assembly since P1.06 and minimal: Thumb-1 can only store r0–r7, so the high registers go through low ones |

- `memcpy` copies strictly forwards, its tail included, so `memmove` can
  hand it every forward copy (dst below src).
- `memcpy` is the only function that may write below the source while
  reading ahead. Its words are 4 bytes apart at least when the phases match,
  and its bytes go one at a time.
- `strlen` saves r4 and returns with `pop {r4, pc}` (8 bytes of stack).

The C files stay in `libc/string` for the other cores. `libc/string/subdir.mk`
drops a C file when the core's kernel directory has a `.S` of the same name.

## Tests

`test/unit/string_kernels`, on every board:
- **memcpy and memmove** (disjoint): every source and destination offset
  modulo 8, every length 0–80, and 127–129 and 255–257. Guard bytes are
  checked on both sides, and so is the return value.
- **memmove** overlapping in one buffer: every pair of offsets 0–23 (both
  directions, equal pointers included), lengths 0–80, against a copy through
  a temporary.
- **memset:** offsets 0–7, lengths 0–80 and the long ones, with c = 0, 0xff,
  0x5a, 0x1a5, 0x15a, 0x7fffff00 and −1. Only the low byte counts
  (7.26.6.1).
- **strlen:** offsets 0–7, lengths 0–80, the string made of 0x01, 0x80,
  0xff, 0x7f, 0x81, 0xfe and 0x41 in seven rotations. Those are the bytes
  that fool a careless zero-byte test.
- **strcmp:** offsets 0–7 × 0–7, lengths 0–40:
  - equal strings;
  - a difference at every position with the pairs (0x41, 0x42),
    (0x7f, 0x80), (0x01, 0xff) and (0x80, 0x81), in both argument orders,
    compared as unsigned char;
  - one string a prefix of the other.

libc-test's string tests and `test/unit/string` run as before.

## Fault injection (Thumb-1, armv6-m under qemu)

- caught, 10:
  - `memcpy`'s tail count not restored;
  - its phase check dropped (qemu raises a bus error on the misaligned
    `ldmia`);
  - its head loop skipped (bus error);
  - `memmove`'s directions swapped;
  - its word tail count off by one;
  - `memset`'s tail count off by one;
  - `uxtb` dropped. The first test values hid it: 0x1a5 replicates to the
    same word by chance. 0x15a and 0x7fffff00 were added for it.
  - `strcmp`'s sign swapped;
  - `strlen` backing up 3 instead of 4.
- equivalent, 2: `strlen` without `bics`, and with 0x40404040 as the high
  mask. Both only add false positives, which the byte rescan absorbs; only
  the speed changes.
- The first version of `memmove` hung when dst = src. Distinct pointers
  with equal phases are at least 4 bytes apart, which the backward head
  loop relied on, but dst = src is not. The overlap test found it, and
  equal pointers now return at once.

Note: a stale `strlen.o` from the C file stayed in the build tree once
`strlen.S` existed, and the archive took the C one. That is the D57
stale-object hazard again. Kernel changes are measured after
`rm -rf build/<board>`.
