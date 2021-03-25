#ifndef EVAL_H
#define EVAL_H

#define E_EXIT 0x01   /* exit after evaluating tree */
#define E_ROOT 0x02   /* we're in the rootnode */
#define E_BQUOTE 0x04 /* backquoted command */
#define E_JCTL 0x08   /* job control */
#define E_LIST 0x10   /* evaluate a node list */
#define E_BREAK 0x20
#define E_CONTINUE 0x40

#include "tree.h"
#include <setjmp.h>
#include <stdlib.h>

struct fdtable;
struct vartab;

struct eval {
  struct eval* parent;
  unsigned char flags;
  short exitcode; /* exit code of last evaluated node */

  struct fdstack* fdstack;
  struct vartab* varstack;

  jmp_buf jumpbuf;
  int jump;

  struct location pos;
};

extern struct eval* eval;

int eval_command(struct eval* e, union node* node, int tempflags);
void eval_jump(int levels, int cont);
int eval_node(struct eval* e, union node* node);
int eval_pop(struct eval* e);
void eval_push(struct eval* e, int flags);
int eval_tree(struct eval* e, union node* node, int tempflags);

int eval_and_or(struct eval* e, struct nandor* nandor);
int eval_case(struct eval* e, struct ncase* ncase);
int eval_cmdlist(struct eval* e, struct ngrp* grp);

int eval_for(struct eval* e, struct nfor* nfor);
int eval_if(struct eval* e, struct nif* nif);
int eval_loop(struct eval* e, struct nloop* nloop);
int eval_pipeline(struct eval* e, struct npipe* npipe);
int eval_simple_command(struct eval* e, struct ncmd* ncmd);
int eval_subshell(struct eval* e, struct ngrp* ngrp);
int eval_function(struct eval* e, struct nfunc* func);

#endif /* EVAL_H */
