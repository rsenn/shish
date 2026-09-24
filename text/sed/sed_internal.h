/* internal to the files in this directory: the compiled program and
 * runtime state. Not installed, not included from outside text/sed/.
 * ----------------------------------------------------------------------- */
#ifndef TEXT_SED_INTERNAL_H
#define TEXT_SED_INTERNAL_H

#include "../dfa.h"
#include "../sed.h"
#include "../../lib/stralloc.h"

enum sed_addr_type { SA_NONE = 0, SA_LINE, SA_LAST, SA_REGEX };

struct sed_addr {
  int type;
  unsigned long line; /* SA_LINE */
  struct dfa re;       /* SA_REGEX */
  int re_set;          /* 0 for an empty //: reuse the last RE used at run time */
};

/* one command's payload, by letter; only the member matching the
 * command's letter is valid.
 * ----------------------------------------------------------------------- */
struct sed_subst {
  struct dfa re;
  int re_set; /* 0 for s//repl/: reuse the last RE used at run time */
  struct dfa_repl* repl;
  unsigned global : 1;
  unsigned print : 1;
  unsigned icase : 1;
  unsigned nth; /* 0 means "1" (POSIX default: first match) */
  int wfile;    /* index from sed_wfile_*(), or -1 */
};

struct sed_text {
  char* s;
  size_t len;
};

struct sed_cmd {
  unsigned char letter;
  unsigned char naddr;
  unsigned char negate;
  struct sed_addr a1, a2;
  size_t jump; /* '{': index just past the matching '}'. b/t: target command index. */

  /* runtime-mutable 2-address range state; one compiled program is
     executed by at most one sed_state at a time, so this lives here
     rather than in a parallel array. */
  unsigned char in_range;

  union {
    struct sed_subst s;
    unsigned char y[256]; /* y///: y.map[c] is c's replacement, identity by default */
    struct sed_text text; /* a i c */
    int wfile;             /* w: index from sed_wfile_*() */
    char* rfile;            /* r: file name, verbatim */
    int qstatus;             /* q: exit status (0 if none given) */
  } u;
};

struct sed_wfile_entry {
  char* name;
  size_t len;
};

struct sed {
  struct sed_cmd* cmds;
  size_t ncmd;
  unsigned flags;
  int autoprint_off; /* #n */
  struct sed_wfile_entry* wfiles;
  size_t nwfiles;
};

/* queued 'a'/'r' output, flushed just before the next line is read
 * (by n/N or by starting a new cycle); 'i' writes immediately instead
 * and never goes through this queue.
 * ----------------------------------------------------------------------- */
struct sed_pending {
  int is_file; /* 0: text (a); 1: file name (r) */
  char* data;
  size_t len;
  struct sed_pending* next;
};

struct sed_state {
  struct sed* prog;
  sed_read_fn read;
  sed_out_fn out;
  sed_wfile_fn wfile;
  sed_rfile_fn rfile;
  void* ctx;

  stralloc pattern;
  stralloc hold;
  unsigned long lineno;
  int cur_had_nl;   /* did the line now in the pattern space's source end with '\n'? */
  int cur_is_last;  /* is the line now in the pattern space the last line of input? */
  int suppress_print; /* d/D/c/n-without-autoprint: skip the end-of-cycle auto-print */
  int tflag;             /* any successful s/// since the last input line or last t/T */
  struct dfa* last_re;    /* last regex actually applied (address or s///), for // */

  /* one-line read-ahead, so is_last (POSIX '$') is known as soon as a
     line becomes the pattern space, for the very first line and for
     every line n/N brings in. */
  stralloc nextbuf;
  int next_valid;
  int next_had_nl;

  struct sed_pending *pending_head, **pending_tail;

  int quit; /* q/Q seen, or n/N ran out of input: sed_run() should stop */
  int quit_status;
};

/* sed_parse.c */
int sed_parse(struct sed* prog, const char* script, size_t len, unsigned flags);

/* sed_addr.c */
int sed_addr_parse(const char** p, const char* end, struct sed_addr* a, unsigned flags);
void sed_addr_free(struct sed_addr* a);
int sed_addr_match(struct sed_addr* a, struct sed_state* st);
int sed_range_match(struct sed_cmd* c, struct sed_state* st);

/* sed_text.c */
int sed_text_parse(const char** p, const char* end, struct sed_text* t);

/* sed_subst.c */
int sed_subst_parse(const char** p, const char* end, struct sed_subst* s, unsigned flags,
                     struct sed* prog);
void sed_subst_free(struct sed_subst* s);
int sed_y_parse(const char** p, const char* end, unsigned char map[256]);
int sed_subst_exec(struct sed_subst* s, struct sed_state* st);

/* sed_wfile.c */
int sed_wfile_intern(struct sed* prog, const char* name, size_t len);

/* sed_cycle.c */
void sed_pending_push(struct sed_state* st, int is_file, const char* data, size_t len);
void sed_pending_flush(struct sed_state* st);
void sed_pending_clear(struct sed_state* st); /* frees queued items without emitting them */

/* sed_list.c */
void sed_do_list(struct sed_state* st, const char* s, size_t n);

#endif
