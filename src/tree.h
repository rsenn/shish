#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include "stralloc.h"
#include "uint64.h"

/* the section numbers refer to the IEEE P1003.2 Draft D11.2 */

enum nod_id {
/* simple   */ N_SIMPLECMD = 0,
/* pipeline */ N_PIPELINE,       /* execute cmd list, while passing 
                                    stdout amongst them */
/* list     */ N_AND,            /* execute cmd2 if cmd succeeded */
               N_OR,             /* execute cmd2 if cmd failed */
//               N_SEMI,           /* execute both */
               N_NOT,            /* execute cmd and negate return status */
/* compound */ N_SUBSHELL,       /* execute the list in a subshell */
               N_CMDLIST,        /* execute the list in the current env */
/* for      */ N_FOR,            /* execute body for each arg,
                                    while putting the arg into var */
/* case */     N_CASE,           /* execute the case matching the expr */
/* casenode */ N_CASENODE,
/* if */       N_IF,             /* execute ontrue/onfalse, depending on
                                    what test evaluates to */
/* loop */     N_WHILE,          /* execute body while test evals to true */
               N_UNTIL,          /* execute body until test evals to true */
/* function */ N_FUNCTION,
/* argument */ N_ARG,
/* assign   */ N_ASSIGN,
/* redir    */ N_REDIR,
    
               N_ARGSTR,
               N_ARGCMD,
               N_ARGPARAM,
               N_ARGARITH,

               N_ARITH_NUM,
               N_ARITH_VAR,
               N_ARITH_PAREN,
               N_ARITH_OR,
               N_ARITH_AND,
               N_ARITH_BOR,
               N_ARITH_BXOR,
               N_ARITH_BAND,
               N_ARITH_EQ,
               N_ARITH_NE,
               N_ARITH_LT,
               N_ARITH_GT,
               N_ARITH_GE,
               N_ARITH_LE,
               N_ARITH_LSHIFT,
               N_ARITH_RSHIFT,
               N_ARITH_ADD,
               N_ARITH_SUB,
               N_ARITH_MUL,
               N_ARITH_DIV,
               N_ARITH_REM,
               N_ARITH_EXP,
               N_ARITH_UNARYMINUS,
               N_ARITH_UNARYPLUS,
               N_ARITH_NOT,
               N_ARITH_BNOT,
};

/* 3.9.1 - simple command
 * ----------------------------------------------------------------------- */

/* the simple command and the compound commands in 3.9.4 have the bgnd and
   rdir members in common, because they all can be redirected and put in
   background */
struct ncmd {
  int          id;
  union node  *next;
  int          bgnd;     /* run in background */
  union node  *rdir;     /* redirections */
  union node  *args;     /* arguments */
  union node  *vars;     /* cmd-local variable assigns */
};

/* 3.9.2 - pipeline
 * ----------------------------------------------------------------------- */
struct npipe {
  int         id;
  union node *next;
  int         bgnd;
  union node *cmds;
  int         ncmd;
};

/* 3.9.3 - lists
 * ----------------------------------------------------------------------- */

/* AND-OR list */
struct nandor {
  int         id;
  union node *next;
  int         bgnd;
  union node *cmd0;
  union node *cmd1;
};

/* list */
struct nlist {
  int         id;
  union node *next;
  int         bgnd;
  union node *cmds;
};

/* the compound list is simply done with node->next, because we don't
   need any further information */

/* 3.9.4.1 - grouping compound
 * ----------------------------------------------------------------------- */
struct ngrp {
  int         id;
  union node *next;
  int         bgnd;     /* run in background */
  union node *rdir;     /* redirections */
  union node *cmds;
};

/* 3.9.4.2 - for loop
 * ----------------------------------------------------------------------- */
struct nfor {
  int         id;
  union node *next;
  int         bgnd;     /* run in background */
  union node *rdir;     /* redirections */
  union node *cmds;
  union node *args;
  char       *varn;
};

/* 3.9.4.3 - case conditional 
 * ----------------------------------------------------------------------- */
struct ncase {
  int         id;
  union node *next;
  int         bgnd;     /* run in background */
  union node *rdir;     /* redirections */
  union node *list;
  union node *word;
};

struct ncasenode {
  int         id;
  union node *next;
  union node *pats;
  union node *cmds;
};

/* 3.9.4.4 - if conditional
 * ----------------------------------------------------------------------- */
struct nif {
  int         id;
  union node *next;
  int         bgnd;     /* run in background */
  union node *rdir;     /* redirections */
  union node *cmd0;
  union node *cmd1;
  union node *test;
};

/* 3.9.4.5 while loop
 * 3.9.4.6 until loop
 * ----------------------------------------------------------------------- */
struct nloop {
  int         id;
  union node *next;
  int         bgnd;     /* run in background */
  union node *rdir;     /* redirections */
  union node *cmds;
  union node *test;
};

/* 3.9.5 function definition 
 * ----------------------------------------------------------------------- */
struct nfunc {
  int         id;
  union node *next;
  union node *cmds;
  char       *name;
};

/* internally used nodes 
 * ----------------------------------------------------------------------- */
struct list {
  enum nod_id  id;
  union node  *next;
};

/* those are a subset of T_WORD
 * 
 * a word is either a redirection, an argument or an assignment.
 * the members nredir.file and nassign.args are themselves a narg.
 * 
 * ----------------------------------------------------------------------- */
struct narg {
  int          id;
  union node  *next;
  union node  *list;
  int          flag;
  stralloc     stra;
};

/* [fd]<operator><file> */
struct nredir {
  int          id;
  union node  *next;
  union node  *list; /* can be file, fd, delim, here-doc-data */
  int          flag;
  union node  *data; /* next here-doc or expansion */
  int          fdes;
  struct fd   *fd;
};

struct nassign {
  int         id;
  union node *next;
  union node *list;
  stralloc    stra;
};

/* argument (word) subnodes
 * ----------------------------------------------------------------------- */
struct nargstr {
  int          id;
  union node  *next;
  int          flag;
  stralloc     stra;
};

struct nargparam {
  int          id;
  union node  *next;
  int          flag;
  char        *name;
  union node  *word;
  int          numb;
};

struct nargcmd {
  int         id;
  union node *next;
  int         flag;
  union node *list;
};

struct nargarith {
  int         id;
  union node *next;
  int         flag;
  union node *tree;
};

struct narithnum
{
  int         id;
  union node *next;
  int64       num;
};

struct narithvar
{
  int         id;
  union node *next;
  const char *var;
};

struct narithunary
{
  int         id;
  union node *next;
  union node *node;
};

struct narithbinary
{
  int         id;
  union node *next;
  union node *left;
  union node *right;
};

/* ----------------------------------------------------------------------- */
union node {
  enum nod_id      id;
  struct list      list;
  struct ncmd      ncmd;
  struct npipe     npipe;
  struct nandor    nandor;
  struct nlist     nlist;
  struct ngrp      ngrp;
  struct nfor      nfor;
  struct ncase     ncase;
  struct ncasenode ncasenode; 
  struct nif       nif;
  struct nloop     nloop;
  struct nfunc     nfunc;
  struct narg      narg;
  struct nredir    nredir;
  struct nassign   nassign;
  struct nargstr   nargstr;
  struct nargcmd   nargcmd;
  struct nargarith nargarith;
  struct nargparam nargparam;
  struct narithnum narithnum;
  struct narithvar narithvar;
  struct narithunary narithunary;
  struct narithbinary narithbinary;
};

/* link node to the branch nptr points to */
#define tree_link(node, nptr) \
  do { \
  *(nptr) = (node); \
    nptr  =&(node)->list.next; \
  } while(0);

/* move node to the node nptr points to */
#define tree_move(node, nptr) \
  do { \
  *(nptr) = (node); \
    nptr  =&(node)->list.next; \
   (node) = NULL; \
  } while(0);

/* skip to the next node */
#define tree_next(nptr) \
   (nptr) = &(*(nptr))->nlist.next;

/*
 * initialize a branch in the tree.
 * node is where the branch starts and
 * nptr will be used as pointer to the
 * current node in conjunction with 
 * tree_link 
 */
#define tree_init(node, nptr) \
  do { \
   (node) = NULL; \
   (nptr) = &(node); \
  } while(0);

#define tree_initn(node, nptr, init) \
  do { \
   (node) = (init); \
   (nptr) = &(node); \
  } while(0);

#ifdef DEBUG
union node *tree_newnodedebug(const char *file, unsigned int line, enum nod_id nod);
#define tree_newnode(id) tree_newnodedebug(__FILE__, __LINE__, (id))
#else
union node *tree_newnode(enum nod_id nod);
#endif /* DEBUG */

void tree_delnode(union node *node);
void tree_free(union node *list);
void tree_print(union node *node, stralloc *sa);
void tree_printlist(union node *node, stralloc *sa, const char *sep);
union node *tree_newlink(union node **nptr, enum nod_id nod);
unsigned int tree_count(union node *node);

#endif /* TREE_H */

