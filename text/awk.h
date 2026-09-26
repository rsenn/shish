/**
 * @defgroup   awk
 * @brief      awk module: a compiled POSIX awk program plus its
 *             tree-walking interpreter.
 *
 * text/awk has no shell dependency beyond the lib/ primitives every
 * text/ engine already uses (buffer, stralloc, open, arena, dfa).
 * Like text/sed, it is pull/push based through a small set of
 * caller-supplied callbacks instead of touching a shell fd table
 * directly: awk_run() asks the caller for the next input record
 * source (main-input operands are the caller's concern: '-'/none
 * means the shell's own stdin) and hands output lines back through
 * callbacks. src/builtin/extra/builtin_awk.c is the thin shell-facing
 * client: option parsing, ARGV/ENVIRON setup, opening real files.
 *
 * Not implemented (documented, see TODO.md Goal 11): `cmd | getline`,
 * `print | cmd`, `system()` (all three need the shell's own process
 * model, which text/ must not depend on -- they are wired through
 * awk_io.run_shell, NULL by default, a runtime error when a program
 * uses them and the caller left it unset), `nextfile`, regex `RS`,
 * character (vs. byte) semantics.
 * @{
 */
#ifndef TEXT_AWK_H
#define TEXT_AWK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct awk_prog; /* opaque: a compiled program */

/* awk_compile()/awk_error() codes (0 = success) */
enum {
  AWK_OK = 0,
  AWK_ENOMEM,  /* allocation failed */
  AWK_ESYNTAX, /* parse error; awk_compile() also fills *errline */
  AWK_EREGEX   /* a /re/ literal failed to compile */
};

/* awk_compile: src/len need not be NUL-terminated; already the
 * concatenation of every -f fragment (POSIX: each fragment ends in a
 * newline before the next is appended). On success *out is a new
 * compiled program and the return is AWK_OK; on error *out is NULL,
 * *errline (if non-NULL) gets the 1-based source line of the error,
 * and the return is one of the codes above. */
int awk_compile(struct awk_prog** out, const char* src, size_t len, unsigned long* errline);
void awk_free(struct awk_prog* prog);
const char* awk_error(int code);

/* awk_io: everything the running program needs from the outside
 * world, gathered in one struct so awk_run() takes a single pointer.
 * There is deliberately no "give me the next file operand" callback:
 * POSIX makes ARGV/ARGC an ordinary awk-visible (and -writable) array,
 * so awk_run() itself populates ARGV from its `operands` argument and
 * the main-input loop (awk_run.c) walks ARGV like any other array,
 * the same way a running program is allowed to.
 *
 *   open_read      opens `name` for a file operand, getline<file or
 *                  cmd|getline (is_cmd nonzero for the latter -- a
 *                  shell command line, run via run_shell); "-" means
 *                  the caller's own stdin. Returns an opaque handle,
 *                  or NULL on failure (getline reports -1, not fatal).
 *   read           like read(2) on a handle from open_read: >0 bytes
 *                  read, 0 at EOF, <0 on error.
 *   close_read     releases a handle from open_read
 *   open_write     opens `name` for `>` (trunc, first use only,
 *                  append after) or `>>` (append is nonzero for both
 *                  `>>` and every reuse of an already-open `>` target).
 *                  Returns an opaque handle, or NULL on failure.
 *   write          writes n bytes to a handle from open_write/out/err
 *                  (or from run_shell's mode 2); n==0 means "flush".
 *   close_write    releases a handle from open_write
 *   out / err      the program's own stdout/stderr, as handles usable
 *                  directly with write() (`print`/`printf` with no
 *                  redirection; `awk: ...` diagnostics)
 *   run_shell      backs `cmd | getline [var]`, `print ... | cmd` and
 *                  system(cmd); NULL (the default) makes all three a
 *                  runtime error. mode 0 = system (cmd is the command
 *                  line, *status gets its exit status); mode 1 = open
 *                  cmd's stdout for reading (like open_read, but for a
 *                  command line instead of a filename), *handle set on
 *                  success; mode 2 = open cmd's stdin for writing
 *                  (like open_write), *handle set on success; with
 *                  cmd == NULL, mode 1/2 instead *close* the piped
 *                  command *handle names (waiting for it).
 * ----------------------------------------------------------------------- */
struct awk_io {
  void* (*open_read)(void* ctx, const char* name);
  long (*read)(void* ctx, void* h, char* buf, size_t len);
  void (*close_read)(void* ctx, void* h);
  void* (*open_write)(void* ctx, const char* name, int append);
  int (*write)(void* ctx, void* h, const char* s, size_t n);
  void (*close_write)(void* ctx, void* h);
  int (*run_shell)(void* ctx, const char* cmd, int mode, void** handle, int* status);
  void* out;
  void* err;
  void* ctx;
};

/* awk_run: runs BEGIN, then (unless the program is BEGIN-only and
 * never reads $0/getline/main input) one pass over ARGV, then END.
 * `assigns` is a NULL-terminated array of "-v"-style "name=value"
 * strings applied before BEGIN; `operands` is the NULL-terminated
 * operand list (files and/or "name=value" assignments) that seeds
 * ARGV[1..]/ARGC, in the shell's own argv order; fs is -F's
 * separator, or NULL for the default. Returns the program's exit
 * status (an explicit `exit expr`, else 0, else 2 on a runtime
 * error). */
int awk_run(struct awk_prog* prog,
            const struct awk_io* io,
            char* const* assigns,
            char* const* operands,
            const char* fs);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
