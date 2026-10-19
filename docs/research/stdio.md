# Research: `<stdio.h>` streams

Step: P1.09. Decisions: PLAN D4, D6, D13, D28, D31, D40.

## Sources reviewed

- musl `src/internal/stdio_impl.h` (kraj/musl mirror): `struct _IO_FILE`
  with separate read (`rpos`/`rend`) and write (`wbase`/`wpos`/`wend`)
  windows over one buffer, function pointers for read/write/seek/close
  (so memory streams and files share the code), 8 bytes of pushback space
  reserved before the buffer (`UNGET`), an open-file list for `fflush(NULL)`
  and exit, a recursive lock word, and `BUFSIZ` 1024. About 150 bytes per
  stream on 32-bit plus the buffer.
- picolibc `libc/include/stdio.h` (tinystdio, from avr-libc): a 16-byte
  `struct __file` with a one-character `unget` field, flags, and
  per-character `put`/`get`/`flush` callbacks; buffering is an optional
  extension (`__file_bufio`); the standard streams are usually unbuffered
  device callbacks. Smallest RAM, but every character is a function call
  and a `FILE` cannot be a host file without the extension.
- newlib `struct __sFILE` (BSD 4.4 lineage): `_p`/`_r`/`_w` counters,
  `_ub` pushback buffer, `_lb` line buffer, per-stream `_lock`, a reentrancy
  pointer; roughly 100 bytes per stream, statically allocated pools
  (`__sglue`).
- BSD stdio (Chris Torek's design, FreeBSD `lib/libc/stdio/local.h`): the
  `_r`/`_w` counters make `getc`/`putc` macros a decrement and a pointer
  step; `__sflush`, `__srefill` and the `__SLBF`/`__SNBF` modes.
- The standard: C23 7.23.2 (streams), 7.23.3 (files, buffering, the flush
  of line-buffered output before input), 7.23.5 (`fopen` modes including
  `x`, `freopen`, `setvbuf`), 7.23.7 (character I/O, one character of
  pushback guaranteed), 7.23.9 (positioning), 7.24.4.4 (`exit` flushes and
  closes streams).

## Choices

| Topic | Options | Chosen |
|---|---|---|
| Per-stream RAM | callbacks only (picolibc), full struct (musl, newlib) | One buffer window plus a mode (idle, reading, writing), a handle, flags, one pushback byte, a lock pointer and list links: 36 bytes on 32-bit. No function pointers: the only backend is the seam's handle I/O. |
| Buffers | static per stream, allocated at open, allocated at first use | Allocated with `malloc(BUFSIZ)` at the first transfer, so an unused stream costs no buffer; if the allocation fails the stream becomes unbuffered instead of failing. `setvbuf` can supply a buffer or a size first. |
| `BUFSIZ` | 256 (the C minimum) … 8192 | 256: RAM per stream is the main concern; it only sets the default buffer. |
| Standard streams | — | Static `FILE` objects on handles 0, 1, 2. `stdout` and `stdin` are line buffered and `stderr` unbuffered: whether a handle is interactive is unknown, and C allows full buffering only when it can be determined not to be (7.23.3p7). |
| Pushback | 8 bytes before the buffer (musl), into the buffer, separate byte | One separate byte: C guarantees one character (7.23.7.10p3), and it never writes outside a user buffer (libc-test `regression/setvbuf-unget`). |
| Read/write switching | — | One window used in either direction; switching flushes pending output or gives back unread input by seeking, so a missing `fseek` between them (undefined) still behaves sanely. |
| Line buffering | — | Writing a new-line flushes the whole buffer (C asks for the line to be transmitted; sending what follows it too is allowed). Every line-buffered output stream is flushed before input from an unbuffered or line-buffered stream (7.23.3p3), done before the input function takes its own lock so the lock order stays list-then-stream. |
| Locking | global lock, per-stream lock | Per-stream recursive lock through the seam (D31): fopen'ed streams create one (`__rtos_lock_create`), the standard streams use port-defined static locks (`__rtos_stdin_lock`, `__rtos_stdout_lock`, `__rtos_stderr_lock`), and the open-file list and `tmpnam`'s counter use `__rtos_stdio_lock`. A reader blocked on `stdin` then never blocks `printf` in another thread. |
| Exit | weak hook in `exit`, destructor | A destructor in `__libc_file_write.c`, the object every output path uses, so it is linked exactly when buffered output can exist; `exit` already runs `.fini_array` after the `atexit` handlers (7.24.4.4p4: handlers, then flush and close). It flushes the standard streams and closes every fopen'ed stream. `quick_exit` and `_Exit` do not flush. |
| Temporary files | open + unlink in libc, port anonymous file | The port names candidates (`__rtos_tmpname(buffer, size, index)`), and `tmpfile` opens one with exclusive create plus a "temporary" flag that makes the port remove it when it is closed (Linux unlinks it right after opening). `tmpnam` asks the port for names until one does not exist, with a counter under `__rtos_stdio_lock`; `L_tmpnam` is 32, `TMP_MAX` 10000. |

## The seam's I/O part (finalised here)

```c
#define __RTOS_OPEN_READ 0x01
#define __RTOS_OPEN_WRITE 0x02
#define __RTOS_OPEN_APPEND 0x04
#define __RTOS_OPEN_CREATE 0x08
#define __RTOS_OPEN_TRUNCATE 0x10
#define __RTOS_OPEN_EXCLUSIVE 0x20
#define __RTOS_OPEN_TEMPORARY 0x40
int __rtos_open(const char *path, unsigned int flags);        /* handle or -errno */
int __rtos_close(int handle);
long long __rtos_seek(int handle, long long offset, int whence); /* SEEK_SET/CUR/END; new offset or -errno */
int __rtos_remove(const char *path);
int __rtos_rename(const char *old_path, const char *new_path);
int __rtos_tmpname(char *buffer, size_t size, unsigned int index);
```

`fopen` modes map to these flags: `r` read; `w` write, create, truncate;
`a` write, create, append; `+` adds the other direction; `x` (with `w`)
adds exclusive; `b` is accepted anywhere after the first character and
ignored (no text translation). The Linux port uses `openat`, `close`,
`lseek` (`_llseek` on Arm), `unlinkat` and `renameat`; `port/none` has no
file system and serves only the standard streams.

## Edge cases

- `fopen` with an invalid mode fails with `EINVAL`; `x` without `w`
  is invalid.
- `fread`/`fwrite` with `size * nmemb` overflowing `size_t`, zero sizes;
  short reads set end-of-file or error correctly.
- `ungetc(EOF)` fails; a second `ungetc` without a read fails; `fseek`,
  `fsetpos` and `rewind` discard pushback; `ftell` counts it.
- `fgets` with `n` of 1 (stores only the terminator) and at end of file.
- Output to a read-only stream and input from a write-only stream set the
  error indicator (and `EBADF`).
- `printf` returns a negative value when a write fails.
- Line-buffered `stdout` is flushed before reading `stdin`, and at exit
  even without a newline.

## Test vectors

- libc-test `regression/setvbuf-unget`; `functional/ungetc` needs `fscanf`
  (P1.10); `api/stdio` requires POSIX functions unconditionally (skipped).
- `test/unit/stdio` with a `stdio.run` driver: write, read back, append,
  `x` mode, seek and tell around buffered data and pushback, read/write
  switching, `setvbuf` modes with user buffers, the sticky end-of-file
  indicator on a growing file, `fgets` edge cases, `fread`/`fwrite`
  element counts, `remove`/`rename`, `tmpfile`/`tmpnam`, `freopen`,
  `perror` output, `stdout` flushed at exit without a newline, `stdin`
  read from a pipe, and unbuffered `stderr` overtaking buffered `stdout`.
  Injected faults caught: no flush at exit, pushback missing from `ftell`,
  no line flush, `SEEK_CUR` from the handle's position, partial elements
  counted by `fread`, a wrong `perror` separator, a non-sticky end of file,
  and `ungetc` keeping the end-of-file indicator.
