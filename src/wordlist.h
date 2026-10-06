#ifndef WORDLIST_H
#define WORDLIST_H

#include "../lib/arena.h"
#include "../lib/stralloc.h"

/* state of a field, and the modes wordlist_cat() was called with */
#define X_DEFAULT 0x00000000
#define X_NOSPLIT 0x01000000
/* chunk straight from source text (N_ARGSTR): gets one expand_unescape(parse_isesc) pass.
   Never set on substitution results: a second pass would eat a real backslash */
#define X_LITERAL 0x02000000
#define X_GLOB 0x04000000
/* the field holds the raw result of an unquoted expansion ($x, $(cmd)): glob it if it has a
   pattern character, and leave it exactly as is when nothing matches */
#define X_GLOBRES 0x00200000
#define X_QUOTED 0x08000000
/* result is already fully processed (unescaped, if needed) by
   wordlist_cat()'s non-splitting branch -- skip any later whole-buffer
   expand_unescape() pass over it. */
#define X_UNESCAPED 0x10000000
/* result feeds path_fnmatch() (case, ${v%pat}): keep the parser's backslash doubling,
   it is path_fnmatch()'s "literal, not a wildcard" escape */
#define X_PATTERN 0x20000000
/* the word of "${parameter+word}" / "${parameter-word}": its literal text is splittable,
   like any expansion result (a command word's literal text is not) */
#define X_SUBWORD 0x00100000
/* set on every field of an unquoted word that field-splitting split
   into 2+ fields (including the first, retroactively). Tells
   expand_argv() to keep an empty field that's one of several real
   fields, while still dropping a word's sole, entirely-empty result. */
#define X_SPLIT 0x40000000


#define WORDLIST_INLINE 16

/* output of word expansion: an append-only sink that turns text chunks into fields.
 * Knows nothing of the parse tree, variables or $(...); the caller passes IFS in.
 *
 *   field mode   closed fields are frozen in ar, v is the argv
 *   string mode  (ar == NULL) every chunk is appended to the caller's stralloc, no breaks
 * ----------------------------------------------------------------------- */
typedef struct wordlist {
  arena* ar;             /* closed fields are frozen here; the caller rewinds it */
  stralloc* cur;         /* the open field: a pooled buffer, or the caller's stralloc in string mode */
  char** v;              /* closed fields, v[n] == NULL */
  size_t n, a;           /* fields in v, slots in v (a >= n + 1) */
  size_t mark;           /* n at the start of the current word */
  unsigned state;        /* X_* bits of the open field */
  const char* ifs;       /* splitting characters, NULL = "" = no splitting */
  unsigned has : 1;      /* a field exists: open, or closed (see closed) */
  unsigned closed : 1;   /* that field is already in v; the next chunk starts a sibling */
  unsigned pend : 1;     /* v[n - 1] is an empty field that is dropped unless a sibling follows */
  unsigned noglob : 1;   /* set -f: patterns stay literal */
  unsigned oom : 1;      /* an allocation failed; the field that needed it was lost */
  stralloc own;          /* cur when the pool could not grow (out of memory) */
  char* inl[WORDLIST_INLINE]; /* v starts here: up to 15 fields cost no malloc */
} wordlist;

/* setup and teardown
 * ----------------------------------------------------------------------- */
void wordlist_init(wordlist* wl, arena* ar, const char* ifs);
void wordlist_init_str(wordlist* wl, stralloc* out); /* appends to out; the caller zeroes it first if needed */
void wordlist_free(wordlist* wl);                   /* give cur back to the pool, free a spilled v */

/* building
 * ----------------------------------------------------------------------- */
void wordlist_cat(wordlist* wl, const char* b, size_t len, int flags); /* append a chunk, splitting it at IFS where the flags allow */
void wordlist_break(wordlist* wl);                                      /* end the open field as it is ("$@" between parameters) */
int wordlist_close(wordlist* wl);                                       /* end of word: glob, unescape, keep or drop; fields added */

/* reading
 * ----------------------------------------------------------------------- */
char** wordlist_argv(wordlist* wl, int* argc); /* v, NULL-terminated, no copy */

/* internal, shared by the files of this module
 * ----------------------------------------------------------------------- */
stralloc* wordlist_pool_get(void);     /* a pooled buffer, NULL when out of memory */
int wordlist_pool_put(stralloc* sa);   /* 1 if sa was the newest pooled buffer and is back */
size_t wordlist_pool_mark(void);       /* buffers in use now */
void wordlist_pool_release(size_t mark); /* a longjmp skipped the frees: back to mark */
void wordlist_push(wordlist* wl); /* freeze cur as the next field, empty cur */
void wordlist_settle(wordlist* wl); /* resolve a pending empty field: drop it */
void wordlist_glob(wordlist* wl, int flags); /* pattern in cur: push all matches but the last, leave that one in cur */

#endif
