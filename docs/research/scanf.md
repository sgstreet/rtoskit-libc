# Research: the `scanf` family

Step: P1.10. Decisions: PLAN D13, D28, D38, D40, D41.

## Sources reviewed

- musl `src/stdio/vfscanf.c` (kraj/musl mirror): one engine for every
  source; `shlim`/`shgetc`/`shunget` limit a field to its width and push back
  one character; the scanset is a 257-entry table; integers go through the
  shared `__intscan` and floats through `__floatscan`, which consume the
  longest prefix and fail when it is not a complete number.
- FreeBSD `lib/libc/stdio/vfscanf.c`: numbers are first collected into a
  513-byte buffer (`BUF`); `parsefloat` keeps a "commit point" and calls
  `ungetc` for everything after it, which needs more than one character of
  pushback (BSD streams allow that); `__sccl` builds the scanset.
- glibc (behaviour only, through the host's `sscanf`): `"0x"` with `%x`
  gives a matching failure; `%%` skips leading white space.
- The standard: C23 7.23.6.2 (`fscanf`: directives, white space, the input
  item as "the longest sequence of input characters which does not exceed
  any specified field width and which is, or is a prefix of, a matching
  input sequence" (p9), input versus matching failure, `%n`, the `%[`
  scanset, `*`, and the C23 `b` conversion and `wN`/`wfN` modifiers),
  7.23.7.11 (one character of pushback), 7.24.1.5 and 7.24.1.7 (the
  subject sequences numbers must match).

## Choices

| Topic | Options | Chosen |
|---|---|---|
| Engine | one per source, one shared | One engine `__libc_scan()` over a small input abstraction (a string, or a locked `FILE` read through `__libc_file_getc`). |
| Pushback | multi-character (BSD), one character (musl) | One character, which is all a `FILE` here keeps (D40): a field reads characters while they still form a prefix of a valid sequence and pushes back only the first one that does not. Hence `"10e"` with `%lf` consumes `10e` and fails, as the standard's p9 wording and libc-test require. |
| Integers | collect then `strtol`, scan directly | A state machine reads the optional sign, the base prefix (`0x` for `x`/`i`/`p`, `0b` for `b`/`i`, a leading `0` meaning octal for `i`) and digits, accumulating in `uintmax_t`; a prefix with no digit after it (`"0x"`) is a matching failure. Overflow is undefined; the value wraps as `strtoumax` would. |
| Floats | collect raw text, normalise | The same prefix rule over `[sign] (inf[inity] \| nan[(chars)] \| 0x hex [p exp] \| dec [e exp])`, but kept in a normal form: significant digits only (up to 900, then a sticky digit), with the point folded into the exponent. The text `[-]0.<digits>e<exp>` (or `0x0.<digits>p<exp>`) then goes to `strtof`/`strtod`/`strtold`, so any input length converts correctly rounded. Behind its own symbol `__libc_scan_float` so a no-float flavour can replace it (D13). |
| Scanset | 256-bit set | A 256-bit set; `]` first (after an optional `^`) is literal; `a-z` with `-` neither first nor last is a range (implementation-defined, C23 p12). |
| Return | — | The number of assigned items, or `EOF` when an input failure happens before the first conversion completes (p16); `%n` and suppressed conversions do not count as assigned but do count as completed. |
| Wide | — | `%lc`, `%ls`, `%l[` are out of scope (PLAN §1): a conversion failure with `errno = EILSEQ`. |

## Edge cases

- White space in the format matches any amount of input white space,
  including none; every conversion except `%c`, `%[` and `%n` skips leading
  white space, `%%` included.
- A literal that does not match is pushed back: matching failure; end of
  input at a literal before any conversion: `EOF`.
- `%c` without a width reads one character and never adds a terminator
  (libc-test `scanf-nullbyte-char`); `%5c` with fewer than 5 characters
  left fails.
- `%n` stores the characters consumed so far (`scanf-bytes-consumed`).
- Width counts the sign and prefix: `%2i` of `"0x34"` is `"0x"`, a failure.
- `%i` of `"011"` is 9, of `"0b101"` is 5 (C23), `%x` accepts `0x`, `%b`
  accepts `0b`.
- `"0"` with `%f%c`: the float converts, `%c` meets end of input, the result
  is 1 (`sscanf-eof`).
- `fscanf` leaves the first unmatched character in the stream for the next
  read (`functional/ungetc`).

## Test vectors

- libc-test `functional/sscanf`, `functional/ungetc` (with an empty
  test-shim `<unistd.h>`; it includes the header but uses nothing from
  it), `regression/scanf-bytes-consumed`, `scanf-match-literal-eof`,
  `scanf-nullbyte-char`, `sscanf-eof`.
- `test/unit/scanf`: a table of input, format and expected results
  (return value, stored values, `%n` counts) for every conversion and
  modifier, compared against glibc's `sscanf` on the host when the table
  was written; long and exotic float inputs against `strtod`; `fscanf`
  on a file checking what stays unread.
