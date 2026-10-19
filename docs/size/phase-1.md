# Phase 1 size baseline

The input to Phase 2 (PLAN D50). These are release builds (`-Os`) at the
end of P1.17. Each figure is the sum of the `size` columns over the members
of `libc.a` (libm included, D46), and for `example/hello-world` (linked
through `port/linux`). `tools/scripts/size-baseline` produced it; P2.01 replaced that script with
`tools/scripts/size-report` (D51), whose `libc.a` totals on the Arm boards
match this table (text = text + rodata + unwind). The per-arch figures are now in
`docs/size/<arch>.md`.

| Board | libc.a text | libc.a data | libc.a bss | hello-world text | hello-world data | hello-world bss |
|---|---:|---:|---:|---:|---:|---:|
| linux-x86_64 | 188071 | 212 | 1992 | 4591 | 136 | 596 |
| linux-armv6-m | 121409 | 124 | 1068 | 2887 | 76 | 288 |
| linux-armv6s-m | 121409 | 124 | 1068 | 2887 | 76 | 288 |
| linux-armv7e-m | 121457 | 124 | 1068 | 2595 | 76 | 288 |
| linux-armv7e-m-dp | 86545 | 124 | 1068 | 2595 | 76 | 288 |
| linux-armv8-m.main | 120977 | 124 | 1068 | 2579 | 76 | 288 |

libc-test: 219 tests pass on every board in both build types
(`test/libc-test/expected/<board>.txt`). There are no `pending:` entries.
207 tests are skipped as out of scope: 182 POSIX, 15 wide-character and
10 dynamic-loading tests.
