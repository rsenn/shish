#ifndef EVAL_H
#define EVAL_H

#define E_EXIT     0x01 /* exit after evaluating tree */
#define E_ROOT     0x02 /* we're in the rootnode */
#define E_BQUOTE   0x04 /* backquoted command */
#define E_JCTL     0x08 /* job control */
#define E_LIST     0x10 /* evaluate a node list */
#define E_BREAK    0x20
#define E_CONTINUE 0x40

#include <stdlib.h>
#include <setjmp.h>
#include "tree.h"

struct fdtable;
struct vartab;

struct eval {
  struct eval     *parent;
  int              flags;
  int              exitcode; /* exit code of last evaluated node */
  
  struct fdstack  *fdstack;
  struct vartab   *varstack;
  
  jmp_buf          jmpbuf;
  int              jump;
};

extern struct eval *eval;
int eval_pop(struct eval *e);
void eval_push(struct eval *e, int flags);
void eval_jump(int levels, int cont);
int eval_tree(struct eval *e, union node *node, int tempflags);

int eval_command(struct eval *e, union node *node, int tempflags);

int eval_simple_command(struct eval *e, struct ncmd *ncmd);
int eval_pipeline(struct eval *e, struct npipe *npipeline);
int eval_and_or(struct eval *e, struct nandor *nandor);
int eval_subshell(struct eval *e, struct ngrp *ngrp);
int eval_case(struct eval *e, struct ncase *ncase);
int eval_cmdlist(struct eval *e, struct ngrp *ngrp);
int eval_if(struct eval *e, struct nif *nif);
int eval_for(struct eval *e, struct nfor *nfor);
int eval_loop(struct eval *e, struct nloop *nloop);

#endif /* EVAL_H */
