/**
 * @defgroup   sed
 * @brief      sed module: a compiled POSIX sed script plus a line-at-a-
 *             time execution engine.
 *
 * text/sed has no filesystem or shell dependency. It is pull-based
 * (sed_run() calls the caller's sed_read_fn whenever it needs a line,
 * including mid-script for 'n'/'N') rather than fed one line at a
 * time from outside, because sed's own control flow -- not the
 * caller's -- decides when the next line is read. sed_compile() only
 * records the *names* of 'r'/'w'/'s///w' targets (sed_wfile_count()/
 * sed_wfile_name()); output and file writes are reported through
 * caller-supplied callbacks instead of touching stdout or opening
 * files directly. src/builtin/extra/builtin_sed.c is the thin
 * shell-facing client: option parsing, opening files, running it.
 * @{
 */
#ifndef TEXT_SED_H
#define TEXT_SED_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sed; /* opaque: a compiled script */

#define SED_ERE 0x01        /* -E/-r: addresses and s/// use ERE instead of BRE */
#define SED_NOAUTOPRINT 0x02 /* -n: suppress the default output, same effect as a leading #n */

/* sed_compile()/sed_error() codes (0 = success) */
enum {
  SED_OK = 0,
  SED_ENOMEM,  /* allocation failed */
  SED_EADDR,   /* malformed address */
  SED_ECMD,    /* unknown command letter, or wrong number of addresses for it */
  SED_ELABEL,  /* b/t/T targets an undefined label */
  SED_EBRACE,  /* unbalanced { } */
  SED_EDELIM,  /* s/// or y/// missing a delimiter, or y///'s two lists differ in length */
  SED_EUNTERM, /* unterminated s///, a/i/c text, or r/w file name */
  SED_EREGEX,  /* an address's or s///'s pattern failed to compile */
  SED_ESIZE    /* script too large or nested too deep */
};

/* sed_compile: script/len need not be NUL-terminated; already the
 * concatenation of every -e/-f fragment, joined by '\n' (POSIX: each
 * fragment is its own set of complete lines). On success *out is a
 * new compiled program and the return is SED_OK; on error *out is
 * NULL and the return is one of the codes above. */
int sed_compile(struct sed** out, const char* script, size_t len, unsigned flags);
void sed_free(struct sed* prog);
const char* sed_error(int code);

/* did the script's first line read exactly "#n" (the auto -n directive)? */
int sed_autoprint_off(const struct sed* prog);

/* r/w/s///w file targets, deduplicated by name and given a stable
 * index (also the index sed_wfile_fn's `file` argument uses) so a
 * caller can open/truncate each one exactly once, before processing
 * begins (POSIX: "each wfile shall be created before processing
 * begins"). r's targets are not included here -- see sed_rfile_fn. */
size_t sed_wfile_count(const struct sed* prog);
const char* sed_wfile_name(const struct sed* prog, size_t i);

/* sed_read_fn: fetch the next input line. On a line, fills *s/*n
 * (valid until the next call; sed_run copies it out) and *had_nl
 * (did the source line actually end with '\n'?) and returns 1.
 * Returns 0 at end of input, <0 on a read error (treated as end of
 * input). Multiple files are the caller's concern: the callback
 * should move on to the next one transparently. */
typedef int (*sed_read_fn)(void* ctx, const char** s, size_t* n, int* had_nl);

/* sed_out_fn: receives one piece of output text (not necessarily a
 * whole line -- callers buffer/flush as they see fit).
 * sed_wfile_fn: receives bytes to append to w-file `file` (an index
 * from sed_wfile_name()). sed_rfile_fn: 'r file's content should be
 * copied to output verbatim; the caller resolves the name and reads
 * it (a missing file is silently ignored, per POSIX). */
typedef void (*sed_out_fn)(void* ctx, const char* s, size_t n);
typedef void (*sed_wfile_fn)(void* ctx, size_t file, const char* s, size_t n);
typedef void (*sed_rfile_fn)(void* ctx, const char* name);

struct sed_state;

struct sed_state* sed_state_new(struct sed* prog, sed_read_fn read, sed_out_fn out,
                                 sed_wfile_fn wfile, sed_rfile_fn rfile, void* ctx);
void sed_state_free(struct sed_state* st);

/* sed_run: processes the whole input to completion (sed_read_fn
 * returning 0) or until 'q'/'Q'. *exit_status gets q's operand (0 if
 * none given, 0 if the script never quit explicitly). */
void sed_run(struct sed_state* st, int* exit_status);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
