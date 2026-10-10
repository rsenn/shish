#ifndef EVAL_H
#define EVAL_H

enum {
  E_EXIT = (1 << 0),   /* exit after evaluating tree */
  E_ROOT = (1 << 1),   /* we're in the rootnode */
  E_BQUOTE = (1 << 2), /* backquoted command */
  E_JCTL = (1 << 3),   /* job control */
  E_LIST = (1 << 4),   /* evaluate a node list */
  E_FUNCTION = (1 << 5),
  E_LOOP = (1 << 6),
  E_PRINT = (1 << 7),
  E_DEBUG = (1 << 8),
  E_EVAL = (1 << 9),   /* frame of the "eval" builtin: break/continue pass through it */
  E_SOURCE = (1 << 10) /* frame of ".": break/continue unwind it, then go on outward */
};

#include "tree.h"
#include "wordlist.h"
#include <setjmp.h>
#include <stdlib.h>

struct fdtable;
struct vartab;

typedef void debug_callback(union node*);
typedef int exit_callback(int);

struct source;

struct eval {
  struct eval* parent;
  int flags;
  short exitcode; /* exit code of last evaluated node */

  struct fdstack* fdstack;
  struct vartab* varstack;
  /* Source stack depth at push time. `exit` inside `eval` longjmps past
     builtin_eval's source_popfd, so eval_pop has to unwind sources to here
     or the next parse_gettok will read from the consumed eval buffer and
     return EOF immediately — making the parent script silently stop. */
  struct source* source;
  arena_pos apos; /* expand_arena at push time: break/return/exit rewind it to here */
  size_t pool;    /* wordlist buffers in use at push time, likewise */

  jmp_buf jumpbuf;
  int jump;
  int pending; /* E_SOURCE frame: levels of break/continue still to run after unwinding it */
  int pendcont;

  debug_callback* debug;
  exit_callback* destructor;

  struct location pos;
};

extern struct eval* eval;

/* "set -e" is off while evaluating what POSIX exempts: an if/while/until condition, a non-last
 * AND-OR member. A global counter, not a per-frame flag, so it reaches into functions and subshells:
 *   f() { false; }; set -e; f && true     # must not abort inside f */
extern int errexit_suppress;

int eval_command(struct eval* e, union node* node, int tempflags);
void eval_jump(int levels, int cont);
void eval_return(int);
void eval_exit(int);
int eval_node(struct eval* e, union node* node);
int eval_node_bgnd(struct eval* e, union node* node);
int eval_pop(struct eval* e);
void eval_push(struct eval* e, int flags);
int eval_tree(struct eval* e, union node* node, int tempflags);

int eval_and_or(struct eval* e, struct nandor* nandor);
int eval_time(struct eval* e, struct ntime* ntime);
int eval_case(struct eval* e, struct ncase* ncase);
int eval_cmdlist(struct eval* e, struct ngrp* grp);

int eval_for(struct eval* e, struct nfor* nfor);
int eval_if(struct eval* e, struct nif* nif);
int eval_loop(struct eval* e, struct nloop* nloop);
int eval_pipeline(struct eval* e, struct npipe* npipe);
int eval_simple_command(struct eval* e, struct ncmd* ncmd);
int eval_subshell(struct eval* e, struct ngrp* ngrp);
int eval_function(struct eval* e, struct nfunc* func);

#include "sh.h"

static inline unsigned int
eval_depth() {
  struct env* e;
  unsigned int n = 0;

  for(e = sh; e; e = e->parent)
    n++;
  return n;
}

/* the "set -x" line prefix: $PS4 expanded, or "+ " (one '+' per subshell level) when unset */
void eval_print_prefix(struct eval* e, buffer* b);
void eval_print_quoted(const char* x, size_t n, buffer* out);
void eval_print_word(const char* s, size_t n, buffer* out);
size_t eval_print_argv(char** argv, buffer* out);

static inline struct eval*
eval_find(int mask) {
  struct eval* e;

  for(e = eval; e; e = e->parent)
    if((e->flags & mask) == mask)
      return e;
  return 0;
}

#endif /* EVAL_H */
