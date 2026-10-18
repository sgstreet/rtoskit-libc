# Toolchain

Recorded in P1.01 (2026-10-18), Ubuntu 24.04 cloud container.

| Tool | Version | Source | Used for |
|---|---|---|---|
| GCC | 14.2.0 (`gcc-14`) | apt `gcc-14` | x86_64 libc and tests (`-std=c23`; GCC 13 rejects it, see PLAN D9) |
| binutils | 2.42 | apt (default) | x86_64 `as`, `ld`, `ar` |
| qemu-user | 8.2.2 | apt `qemu-user` | runs ARM test binaries from P1.06 |
| clang-format | 18.1.3 | apt `clang-format` | formatting with `tools/clang-format` |
| arm-none-eabi GCC | 15.2 (planned) | `make setup` (the `setup.mk` sketch, red-rocket-computing/cross-dev release v0.2.0), or Arm GNU Toolchain 14.3.rel1 | ARM targets from P1.06. Both download URLs were reachable on 2026-10-18. The apt `gcc-arm-none-eabi` (13.2) is too old for `-std=c23`. |

Cloud sessions install the apt packages through `.claude/hooks/session-start.sh`.

## Notes

- The headers rely on GCC's predefined macros (`__INT64_C(c)`,
  `__SIG_ATOMIC_MIN__`, `__WCHAR_MIN__`, `__WINT_MIN__`, ...). Clang 18 lacks
  some of them (it has `__INT64_C_SUFFIX__` instead). Verify the Clang version
  that provides them before enabling Clang builds (D9).
- `__builtin_flt_rounds` does not exist in GCC 14, so `FLT_ROUNDS` calls the
  library function `__flt_rounds()`.
- Ubuntu's GCC enables PIE, `-fstack-protector-strong` and CET by default.
  `tools/makefiles/x86_64.mk` turns off PIE and the stack protector for the
  static, libc-less link.

## Formatting check

```sh
clang-format --style=file:tools/clang-format --dry-run $(find src arch port include test/unit -name '*.[ch]')
```
