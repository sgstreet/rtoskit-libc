# Research: Arm M-profile bring-up under qemu-user

Step: P1.06. Decisions: PLAN D10, D15, D18, D22, D24, D34, D36, D37 (D35 superseded).

## Sources reviewed

- musl (kraj/musl mirror, <https://github.com/kraj/musl>):
  `arch/arm/syscall_arch.h` (r7 handling in Thumb code: musl moves r7
  aside around `svc 0` because r7 is the Thumb frame pointer and may not be
  an asm operand), `arch/arm/pthread_arch.h` (thread pointer from
  `mrc p15, 0, rX, c13, c0, 3` on v6K/v7+, otherwise the kernel helper at
  `0xffff0fe0`; `TLS_ABOVE_TP` with an 8-byte gap), `src/thread/arm/__aeabi_read_tp.s`,
  `src/setjmp/arm/setjmp.S` (r4–r11, sp, lr, then d8–d15 when the CPU
  has VFP, decided at run time from `__hwcap`).
- picolibc `picocrt/machine/arm/crt0.c`
  (<https://github.com/picolibc/picolibc>): the Linux variant reads argc,
  argv and envp from the initial stack and sets the thread pointer with the
  Arm private call `0xf0005` (`set_tls`), subtracting the 8-byte TCB offset;
  the bare-metal variant enables the FPU in CPACR. Its Arm `setjmp.S` and
  `read_tp.S` could not be fetched (404 at the paths tried).
- Arm, *Run-time ABI for the Arm Architecture* (RTABI) §5.3:
  `__aeabi_read_tp` returns the thread pointer in r0 and preserves the
  other registers; GCC's `load_tp_soft` pattern assumes only r0, ip, lr and
  the flags change.
- Arm, *Procedure Call Standard for the Arm Architecture* (AAPCS) §5.1.1
  (r4–r11 and sp are callee-saved; r7 is the Thumb frame pointer by
  convention), §5.1.2.1 (s16–s31, i.e. d8–d15, are callee-saved), §6.2.1
  (sp 8-byte aligned at public interfaces).
- Arm, *ELF for the Arm Architecture* §4.4 (TLS variant I, two-word TCB at
  the thread pointer; local-exec offsets include the 8-byte gap).
- Linux `arch/arm/kernel/entry-common.S` and `asm/unistd.h` (EABI: number in
  r7, arguments r0–r5, `svc #0`; `__ARM_NR_set_tls` = `0xf0005`), and the
  ELF loader (Thumb entry when `e_entry` bit 0 is set).
- QEMU 8.2 `linux-user/arm`: TPIDRURO emulation and the private calls;
  `qemu-arm -cpu help` lists the M-profile models.
- GCC 15 manual, Arm options (`-march` extensions, `-mtp`), and the
  installed toolchain's `-print-multi-lib`.

## Toolchain

- arm-none-eabi GCC 16.2 (crosstool-NG, `red-rocket-computing/cross-dev`
  v0.3.0; the bring-up started on 15.2 from v0.2.0), downloaded into
  `local/` by `make setup`; the top-level `Makefile` makes `libc` and
  `port` depend on `setup-tools`, so the first build fetches it (pi-pico's
  pattern). The setup target is the versioned driver
  (`arm-none-eabi-gcc-16.2.0`), so a version change reinstalls `local/`.
- Its multilibs cover all five architectures:

| Board | Flags | libgcc multilib |
|---|---|---|
| `linux-armv6-m` | `-mthumb -march=armv6-m -mfloat-abi=soft` | `thumb/v6-m/nofp` |
| `linux-armv6s-m` | `-mthumb -march=armv6s-m -mfloat-abi=soft` | `thumb/v6-m/nofp` |
| `linux-armv7e-m` | `-mthumb -march=armv7e-m+fp -mfloat-abi=hard` | `thumb/v7e-m+fp/hard` |
| `linux-armv7e-m-dp` | `-mthumb -march=armv7e-m+fp.dp -mfloat-abi=hard` | `thumb/v7e-m+dp/hard` |
| `linux-armv8-m.main` | `-mthumb -march=armv8-m.main+fp+dsp -mfloat-abi=hard` | `thumb/v8-m.main+fp/hard` |

- **`armv6-m` and `armv6s-m`** (D37, superseding D35): plain Armv6-M lacks
  `SVC` and GAS rejects it ("SVC is not permitted on this architecture").
  Armv6S-M adds `SVC`; Cortex-M0/M0+ implement it and GCC's
  `-mcpu=cortex-m0` selects it. Both are targets with their own boards;
  they share the v6-m multilib (libgcc is tagged v6S-M, our objects v6-M,
  and ld links them without complaint). libc itself never issues `SVC`;
  the Linux port emits it as raw bits so it assembles for plain Armv6-M.
- GCC 15 does not implement `_BitInt` for 32-bit Arm
  (`'_BitInt(16)' is not supported on this target`, no
  `__BITINT_MAXWIDTH__`); GCC 16.2 does (`__BITINT_MAXWIDTH__` 65535). The
  `_BitInt` checks live in `test/unit/bitint`, which runs on every board
  (Q12, resolved by D37).
- The bare-metal objects carry no `.note.GNU-stack`, so ld warns that the
  stack is executable; the link passes `-z noexecstack`.

## Linux port on Arm (`port/linux/arm/`, shared by the five architectures)

| Piece | Choice |
|---|---|
| Entry | `_start` in Thumb-1 (runs on every M-profile arch): clear lr, pass sp (argc, argv, envp) to `__rtos_linux_start`. The kernel and qemu start Thumb code because the entry address has bit 0 set. |
| System calls | One out-of-line Thumb-1 function `__rtos_linux_syscall(a, b, c, d, e, f, number)` that saves r4, r5, r7, loads the stacked arguments and the number into r7 and issues `svc #0` (as `.inst.n 0xdf00`, D37). Out of line, unlike musl's inline r7 shuffle, so `-O0` frame-pointer use of r7 cannot clash; system call speed does not matter in the test port. `__linux_syscallN` inline wrappers keep the generic port code unchanged. Numbers: read 3, write 4, getpid 20, mmap2 192 (offset 0 only), gettid 224, exit_group 248, tgkill 268. |
| Thread pointer | `set_tls` (`0xf0005`) loads TPIDRURO; `__aeabi_read_tp` (port, D10) reads it with `mrc p15, 0, r0, c13, c0, 3`, emitted as `.inst.w 0xee1d0f70` because M-profile assemblers reject CP15 (keeping the object attributes at the target architecture). It changes only r0. |
| TLS image | The port finds PT_TLS through `__ehdr_start`, as on x86_64. The default arm-none-eabi linker script starts code at the segment base, so the headers are not loaded and `__ehdr_start` is undefined. The boards add `port/linux/arm/headers.ld` (`-T` with `INSERT BEFORE .init`, moving the location counter past `SIZEOF_HEADERS`) through `BOARD_LDFLAGS`; ld then places the headers in the first segment. The auxiliary vector's `AT_PHDR` is no alternative: it is computed from `e_phoff` and points at unmapped memory when the headers are not loaded. |
| ELF structures | `linux-arch.h` declares the ELF32 header and program header (same field names as the x86_64 ELF64 ones, different order and widths). |

## TLS layout (`libc/tls/arm/`)

Variant I (ELF for the Arm Architecture): the thread pointer addresses a
two-word reserved block; the TLS data follows at
`__libc_tls_variant1_offset(image, 8)`, which equals
`align_up(8, p_align)` when the segment address is aligned (what GNU ld
assumes for local-exec) and keeps the image congruent otherwise. The thread
pointer is aligned to `max(p_align, 8)`; the block size is that offset plus
the TLS size plus alignment slack.

## `setjmp`/`longjmp`

- `jmp_buf` stays the one expression from D33, `10 * sizeof(void *) + 64`
  = 104 bytes on Arm: r4–r11, sp, lr and d8–d15 (D15: the same 64 bytes
  serve FPv4-SP, FPv5-SP and FPv5-D16, since s16–s31 alias d8–d15 and
  `VSTM`/`VLDM` of D registers exist on single-precision FPUs).
- `armv6-m`, `armv6s-m` (identical files): Thumb-1 can store only r0–r7, so r8–r11, sp and lr go through
  low registers; `longjmp` restores the high registers first, then r4–r7.
- `armv7e-m`, `armv7e-m-dp`, `armv8-m.main`: `stmia r0!, {r4-r11, ip, lr}`
  (sp via ip), `vstmia r0, {d8-d15}`; the reverse with `it eq` for the
  `val == 0` case. The three files are identical; they are per-arch because
  they depend on both the ISA and the FPU (D34), and a shared FPU directory
  would need a grouping variable for just these two small files. FPSCR is
  not saved (C23 7.13.2.1 does not require it).
- musl decides at run time whether to save the VFP registers; here the
  multilib decides at build time (no hwcap on bare metal).

## `FLT_ROUNDS`

`armv6-m` and `armv6s-m` have no FPU and libgcc's soft-float rounds to nearest: 1. The FPU
variants read FPSCR.RMode (bits 22–23: nearest, +inf, −inf, zero) and map
them to 1, 2, 3, 0.

## qemu-user

- `qemu-arm -cpu max` (`TARGET_RUNNER` in the board files, D36) runs all
  five architectures; its A-profile model accepts every M-profile Thumb
  instruction we generate. It does not reject instructions outside the
  selected architecture, so the assembler and compiler `-march` checks are
  what keep v6-m code Thumb-1 (GAS refused `svc` for `armv6-m`, which shows
  they do).
- The M-profile models (`-cpu cortex-m0`, `cortex-m7`, `cortex-m33`) run
  user code and system calls, but raise SIGILL on the CP15 TPIDRURO read, so
  this port cannot use them: thread-local storage on Linux needs the
  A-profile register. Checked with a probe: `svc` works under `cortex-m0`
  and `cortex-m33`, the `mrc` faults.
- FPv5-D16 under `-cpu max` and `-cpu cortex-m7`: `VRINT{A,N,M,P,Z,R,X}.F64`,
  `VMAXNM`/`VMINNM.F64` with a quiet NaN, fused `VFMA.F64` (single rounding:
  `(1 + 2^-30)(1 - 2^-30) - 1 = -2^-60`), `VSQRT.F64`, `VCVTA.S32.F64` and
  the F32 forms all give the architected results. `test/unit/fpv5` keeps the
  check (built for `LIBC_FPU = fpv5-dp`; an injected wrong expectation and a
  `VMLA` in place of `VFMA` were both caught).

## Findings while bringing up

- GCC 15 elided the unused `malloc` calls in the libc-test shim's
  `t_memfill` (built with builtins) and folded its loop into `b .`; the
  shim now builds with `-fno-builtin`, like the libc-test code.
- A link that failed early left zero-length ELFs that later looked up to
  date; `tools/makefiles/libc-test.mk` now sets `.DELETE_ON_ERROR`.

## Test vectors

- All unit tests and the 21 libc-test tests on every board, debug and
  release; the baselines `expected/linux-<arch>.txt` equal the x86_64 one.
- `test/unit/setjmp` gained a callee-saved check: values held in registers
  across a call to a function that jumps out from below a register-heavy
  callee must survive. Injected faults caught in optimised builds: dropping
  `vldmia` (armv7e-m-dp), a wrong r4–r7 restore (armv6-m), swapped
  `r12`/`r13` slots (x86_64). A swap of r9/r10 on armv6-m is not observable
  from C, because GCC's Thumb-1 code does not allocate values to r8–r11.
- `test/unit/smoke` checks `thread_local` through `__aeabi_read_tp` and the
  copied `.tdata` image on every arch.
