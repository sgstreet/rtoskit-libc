# rtoskit-libc
Small libc implementation for embedded systems.

A clean-room ISO C23 standard library for deeply embedded systems, bound to an
RTOS through a link-time seam (`include/rtos/rtos-port.h`). Licence: MPL-2.0.

- Building and using it: `docs/configuration.md` (archives, flavours, cores, allocators, ports).
- Design and decisions: `docs/plan/PLAN.md`; per-phase task lists in `docs/plan/`.
- Size figures: `docs/size/phase-2.md`, `docs/size/<arch>.md`; accuracy and algorithm notes in `docs/research/`.
- Speed figures: `docs/perf/<arch>.md` (`make bench`, instruction counts under qemu; `docs/research/benchmarks.md`).
