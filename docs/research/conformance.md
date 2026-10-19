# Research: conformance closure for phase 1

Step: P1.17. Decision: PLAN D50.

## Sources reviewed

- The standard text: C23 Annex B (the library summary, `docs/standard/annex-b.pdf`), Annex J.3 and J.4 (implementation-defined and locale-specific behaviour), 4p6 (freestanding implementations), and 6.10.10.3 (the conditional feature macros).
- libc-test `src/api/*.c` (musl's header tests): each declaration is checked by taking a function's address into a pointer of the expected type, compiled with `-pedantic-errors`. They cover only part of C23 and mix in POSIX names.
- glibc's `include/stdc-predef.h`: the hosted GCC driver pre-includes it, and it defines `__STDC_IEC_559__`, `__STDC_IEC_60559_BFP__`, `__STDC_IEC_559_COMPLEX__` and `__STDC_ISO_10646__`. GCC skips the pre-include under `-nostdinc` and `-ffreestanding`.
- musl and picolibc, for documentation of implementation-defined behaviour: musl documents its behaviour piecemeal; picolibc does not document J.3 at all.

## Approaches

| Check | Options | Chosen |
|---|---|---|
| Prototypes | (a) Function pointers of the expected type, as libc-test does. (b) Redeclare each function with its Annex B prototype. | (b). A redeclaration with an incompatible type is a constraint violation, so a mismatch fails to compile. The function name is parenthesised, so a masking macro does not expand. |
| Macros, types, constants | `#ifndef X` alone misses enum constants, typedefs and objects. | `#ifndef X` followed by `static_assert(sizeof(X) > 0)`. This accepts a macro, a type, a constant or an object of the name. `FILE` may be incomplete (C23 6.2.5p1), so it is checked through a pointer. |
| Pseudo-prototypes (`real-floating`, generic placeholders, `QChar`, atomic `A`/`C`/`M`) | Cannot be redeclared. | An existence check: a macro, or a declared function. |
| Freestanding | (a) Compile-only. (b) Link without the port. | (b). The freestanding test links `libc.a` without the port and without startup code, so any dependency on the RTOS seam (errno through TLS, locks, I/O) fails the build. A second link through the port runs the checks. |

## Scope exclusions in the audit

The audit leaves these out:
- `<uchar.h>`, `<wchar.h>` and `<wctype.h>`;
- decimal floating point (DFP);
- Annex K (`_s` functions, `errno_t`, `rsize_t`);
- the macros that are defined only when supported (the `FE_*` exceptions and directions, `FP_FAST_*`, `__STDC_NO_*`, `imaginary`).

These exclusions match PLAN §1, D44 and D49.

## Findings

The audit generated these checks:
- 689 identifiers;
- 642 prototypes;
- 73 generic names across 28 headers.

It found two discrepancies:
- `BOOL_MAX` (C23 5.2.5.3.2) was missing from `<limits.h>`;
- `CR_DECIMAL_DIG` (Annex F.5) was missing from `<float.h>`. It is defined under `__STDC_WANT_IEC_60559_EXT__` as 800. The conversions are correctly rounded for any length (D38, D39).

Fault injection confirmed that the checks bite: removing a macro, changing a prototype's parameter type, or adding a seam call to a freestanding string function each fails the build.
