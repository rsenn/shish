/**
 * @defgroup   dfa
 * @brief      DFA module: POSIX BRE/ERE matching.
 *
 * Compiles a pattern into a flat bytecode program (Thompson NFA:
 * CHAR/ANY/SET/SPLIT/JMP/SAVE/BOL/EOL/BACKREF/MATCH) and matches it
 * against a subject with Pike's thread-based simulation, which gives
 * POSIX leftmost-longest results and never recurses per input byte --
 * bytecode text/dfa/dfa_prog.h can grow this API's implementation
 * without any of these functions changing. Patterns with a
 * back-reference fall back to a bounded backtracker instead (that
 * requirement is fundamental: back-references are not regular).
 * @{
 */
#ifndef TEXT_DFA_H
#define TEXT_DFA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* opaque to callers except for size: every field is filled in by
   dfa_compile()/dfa_free() only. Zero-initialized is the empty/unset
   state, so `static struct dfa re = {0};` needs no other setup. */
struct dfa {
  void* prog;
  size_t proglen;
  void* sets;
  size_t nsets;
  size_t ngroup;
  int has_backref;
  unsigned flags;
};

struct dfa_span {
  size_t start, end; /* end exclusive; start == end is an empty match */
};

/* dfa_compile() flags
 * ----------------------------------------------------------------------- */
#define DFA_ERE 0x01    /* extended RE; default is basic (BRE) */
#define DFA_ICASE 0x02  /* ASCII case-insensitive */
#define DFA_NOTBOL 0x04 /* s[0] is not the start of a line: ^ never matches there */

/* dfa_error() codes; dfa_compile() returns one of these (0 = success) */
enum {
  DFA_OK = 0,
  DFA_ENOMEM,   /* allocation failed */
  DFA_EPAREN,   /* unbalanced \( \) or ( ) */
  DFA_EBRACKET, /* unterminated [ ] */
  DFA_EBRACE,   /* bad or unclosed \{m,n\}, m > n, or m/n too large */
  DFA_ERANGE,   /* reversed range in a bracket expression, e.g. [z-a] */
  DFA_ECLASS,   /* unknown [:class:] name */
  DFA_ESUBREG,  /* \N referring to a group that doesn't exist yet */
  DFA_EBADRPT,  /* repetition operator with nothing to repeat (ERE) */
  DFA_EESCAPE,  /* pattern ends with a trailing backslash */
  DFA_ESIZE     /* pattern or program too large, or nesting too deep */
};

/* dfa_compile: pat/patlen need not be NUL-terminated. *d must be
 * zeroed (e.g. `static struct dfa re = {0};`) before the first call.
 * On success returns DFA_OK; on error returns one of the codes above
 * and leaves *d zeroed/freed, so the caller never has to clean up
 * after a failed compile. Compiling into an already-compiled *d
 * frees the old program first. */
int dfa_compile(struct dfa* d, const char* pat, size_t patlen, unsigned flags);

/* dfa_free: releases *d's internals and zeroes it. A no-op on an
 * already-zeroed struct, so it's always safe to call. */
void dfa_free(struct dfa* d);

/* number of \( \) / ( ) groups in the compiled pattern (0 if none) */
size_t dfa_groups(const struct dfa* d);

/* fixed string for a dfa_compile() error code, no allocation */
const char* dfa_error(int code);

/* dfa_test: is there a match anywhere in s[0..n)? (grep, sed /re/) */
int dfa_test(struct dfa* d, const char* s, size_t n);

/* dfa_prefix: length of the longest match anchored at s[0], or -1 if
 * none (0 is a valid empty match). (POSIX `expr STRING : BRE`) */
long dfa_prefix(struct dfa* d, const char* s, size_t n);

/* dfa_search: leftmost-longest match starting at or after `from`;
 * `^` still means offset 0 in s, not `from` (sed s///g, grep -o). */
int dfa_search(struct dfa* d, const char* s, size_t n, size_t from, struct dfa_span* m);

/* dfa_submatch: fills g[0..ng) with \1..\N's spans inside the match m
 * that dfa_search/dfa_test/dfa_prefix already found (relative to s).
 * A group that did not participate gets {(size_t)-1, (size_t)-1}. */
int dfa_submatch(struct dfa* d,
                 const char* s,
                 size_t n,
                 const struct dfa_span* m,
                 struct dfa_span* g,
                 size_t ng);

/* ---- replacement templates: shared by sed's s/// and (later) awk's
 * sub/gsub -- compiled once per template, applied per match.
 * ----------------------------------------------------------------------- */

struct dfa_repl; /* opaque */

#define DFA_REPL_BACKREF 0x01 /* allow \1-\9 and \n (sed); off = only & \& \\ */

/* dfa_repl_compile: tmpl/len need not be NUL-terminated. On success
 * *out is a new compiled template and the return is DFA_OK; on error
 * *out is NULL and the return is DFA_ENOMEM. */
int dfa_repl_compile(struct dfa_repl** out, const char* tmpl, size_t len, unsigned flags);
void dfa_repl_free(struct dfa_repl* r);

typedef int (*dfa_repl_out_fn)(void* ctx, const char* s, size_t n);

/* dfa_replace: runs the s///[g][N] loop over s[0..n), implementing
 * the empty-match rule (an empty match is followed by copying one
 * literal byte before searching again, so "s,x*,-,g" on "abc" gives
 * "-a-b-c-" instead of looping forever). nth (1-based; 0 means "1"):
 * the first match number to replace; global: replace nth and every
 * match after it, else only the nth. Unmatched spans of s and
 * replacement expansions are both delivered through out(); on
 * success (0) *nreplaced gets the number of replacements made. A
 * nonzero out() return aborts and is passed back as -1, in which case
 * *nreplaced is left untouched. */
int dfa_replace(struct dfa* d,
                const struct dfa_repl* r,
                const char* s,
                size_t n,
                unsigned nth,
                int global,
                dfa_repl_out_fn out,
                void* ctx,
                unsigned* nreplaced);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
