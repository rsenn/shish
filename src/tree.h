#ifndef TREE_H
#define TREE_H

#include "source.h"
#include "../lib/stralloc.h"
#include "../lib/uint8.h"
#include "../lib/uint64.h"
#include <stdlib.h>

#ifndef __packed
#define __packed /*__attribute__((packed))*/
#endif

/* the section numbers refer to the IEEE P1003.2 Draft D11.2 */

enum kind {
  /* simple   */ N_SIMPLECMD = 0,
  /* pipeline */ N_PIPELINE, /* execute cmd list, while passing
                                stdout amongst them */
  /* list     */ N_AND,      /* execute cmd2 if cmd succeeded */
  N_OR,                      /* execute cmd2 if cmd failed */
  // N_SEMI,           /* execute both */
  N_NOT, /* execute cmd and negate return status */
  N_LIST,
  /* compound */ N_SUBSHELL, /* execute the list in a subshell */
  N_BRACEGROUP,              /* execute the list in the current env */
  /* for      */ N_FOR,      /* execute body for each arg,
                                while putting the arg into var */
  /* case */ N_CASE,         /* execute the case matching the expr */
  /* casenode */ N_CASENODE,
  /* if */ N_IF,      /* execute ontrue/onfalse, depending on
                         what test evaluates to */
  /* loop */ N_WHILE, /* execute body while test evals to true */
  N_UNTIL,            /* execute body until test evals to true */
  /* function */ N_FUNCTION,
  /* argument */ N_ARG,
  /* assign   */ N_ASSIGN,
  /* redir    */ N_REDIR,

  N_ARGSTR,
  N_ARGCMD,
  N_ARGPARAM,
  N_ARGARITH,

  A_NUM,
  A_PAREN,
  // binary
  A_OR,
  A_AND,
  A_BITOR,
  A_BITXOR,
  A_BITAND,
  A_EQ,
  A_NE,
  A_LT,
  A_GT,
  A_GE,
  A_LE,
  A_SHL,
  A_SHR,
  A_ADD,
  A_SUB,
  A_MUL,
  A_DIV,
  A_MOD,
  A_EXP,
  // unary
  A_UNARYMINUS,
  A_UNARYPLUS,
  A_NOT,
  A_BNOT,
  A_PREDECR,
  A_PREINCR,
  A_POSTDECR,
  A_POSTINCR,
  A_VASSIGN,
  A_VADD,
  A_VSUB,
  A_VMUL,
  A_VDIV,
  A_VMOD,
  A_VSHL,
  A_VSHR,
  A_VBITAND,
  A_VBITXOR,
  A_VBITOR
};

/* 3.9.1 - simple command
 * ----------------------------------------------------------------------- */

/* the simple command and the compound commands in 3.9.4 have the bgnd and
   rdir members in common, because they all can be redirected and put in
   background */
struct ncmd {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* vars; /* cmd-local variable assigns */
  union node* args; /* arguments */
} __packed;

/* 3.9.2 - pipeline
 * ----------------------------------------------------------------------- */
struct npipe {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  unsigned ncmd;
  union node* cmds;
} __packed;

struct nnot {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* pipeline;
} __packed;

/* 3.9.3 - lists
 * ----------------------------------------------------------------------- */

/* AND-OR list */
struct nandor {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* left;
  union node* right;
} __packed;

/* the compound list is simply done with node->next, because we don't
   need any further information */

/* 3.9.4.1 - grouping compound
 * ----------------------------------------------------------------------- */
struct ngrp {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* cmds;
} __packed;

/* 3.9.4.2 - for loop
 * ----------------------------------------------------------------------- */
struct nfor {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* cmds;
  union node* args;
  char* varn;
} __packed;

/* 3.9.4.3 - case conditional
 * ----------------------------------------------------------------------- */
struct ncase {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* list;
  union node* word;
} __packed;

struct ncasenode {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* pats;
  union node* cmds;
} __packed;

/* 3.9.4.4 - if conditional
 * ----------------------------------------------------------------------- */
struct nif {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* cmd0;
  union node* cmd1;
  union node* test;
} __packed;

/* 3.9.4.5 while loop
 * 3.9.4.6 until loop
 * ----------------------------------------------------------------------- */
struct nloop {
  enum kind id;
  unsigned bgnd : 1; /* run in background */
  union node* next;
  union node* rdir; /* redirections */
  union node* cmds;
  union node* test;
} __packed;

/* 3.9.5 function definition
 * ----------------------------------------------------------------------- */
struct nfunc {
  enum kind id;
  unsigned dummy : 1;
  struct nfunc* next;
  char* name;
  union node* body;
  struct location loc;
} __packed;

/* command list
 * ----------------------------------------------------------------------- */
struct nlist {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* cmds;
} __packed;

/* word nodes
 *
 * a word is either a redirection, an argument or an assignment.
 * ----------------------------------------------------------------------- */
struct narg {
  enum kind id;
  unsigned flag;
  union node* next;
  stralloc stra;
  union node* list;
} __packed;

/* [fd]<operator><file> */
struct nredir {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* word; /* can be file, fd, delim, here-doc-data */
  union node* data; /* next here-doc or expansion */
  int fdes;
  struct fd* fd;
} __packed;

/* argument (word) subnodes
 * ----------------------------------------------------------------------- */
struct nargstr {
  enum kind id;
  unsigned flag;
  union node* next;
  union {
    stralloc stra;
    struct {
      char* str;
      size_t len;
    };
  };
  struct location loc;
} __packed;

struct nargparam {
  enum kind id;
  unsigned flag;
  union node* next;
  char* name;
  union node* word;
  long numb;
  struct location loc;
} __packed;

struct nargcmd {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* list;
} __packed;

struct nargarith {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* tree;
} __packed;

struct narithnum {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  int64 num;
  unsigned base;
} __packed;

struct narithunary {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* node;
} __packed;

struct narithbinary {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* left;
  union node* right;
} __packed;

/* ----------------------------------------------------------------------- */
union node {
  struct {
    enum kind id;
    unsigned dummy : 1;
    union node* next;
  };
  struct nlist nlist;
  struct ncmd ncmd;
  struct npipe npipe;
  struct nandor nandor;
  struct nnot nnot;
  struct ngrp ngrp;
  struct nfor nfor;
  struct ncase ncase;
  struct ncasenode ncasenode;
  struct nif nif;
  struct nloop nloop;
  struct nfunc nfunc;
  struct narg narg;
  struct nredir nredir;
  struct nargstr nargstr;
  struct nargcmd nargcmd;
  struct nargarith nargarith;
  struct nargparam nargparam;
  struct narithnum narithnum;
  struct narithunary narithunary;
  struct narithbinary narithbinary;
};

typedef union node node_t;

/* link node to the branch nptr points to */
#define tree_link(node, nptr) \
  do { \
    *(nptr) = (node); \
    nptr = &(node)->next; \
  } while(0);

/* link node to the branch nptr points to */
#define tree_unshift(node, nptr) \
  do { \
    (node)->next = *(nptr); \
    (*nptr) = (node); \
  } while(0);

/* move node to the node nptr points to */
#define tree_move(node, nptr) \
  do { \
    *(nptr) = (node); \
    nptr = &(node)->next; \
    (node) = NULL; \
  } while(0);

/* skip to the next node */
#define tree_next(nptr) (&((*(nptr)))->next)
#define tree_skip(nptr) ((nptr) = tree_next(nptr))

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

#ifdef DEBUG_ALLOC
union node* tree_newnodedebug(const char* file, unsigned int line, enum kind nod);
#define tree_newnode(id) tree_newnodedebug(__FILE__, __LINE__, (id))
#else
union node* tree_newnode(enum kind nod);
#endif /* DEBUG_ALLOC */

extern const int tree_nodesizes[];

void tree_delnode(union node* node);
void tree_free(union node* list);
void tree_cat(union node* node, stralloc* sa);
void tree_cat_n(union node* node, stralloc* sa, int depth);
void tree_catlist(union node* node, stralloc* sa, const char* sep);
void tree_catlist_n(union node* node, stralloc* sa, const char* sep, int depth);
void tree_catseparator(stralloc* sa, const char* sep, int depth);
union node* tree_newlink(union node** nptr, enum kind nod);
unsigned int tree_count(union node* node);
union node** tree_append(union node**, union node*);
void tree_remove(union node**);
int tree_location(union node*, struct location*);
char* tree_string(union node* node);

const char* node2str(const union node n);

#ifdef BUFFER_H
void tree_print(union node*, buffer*);
void tree_printlist(union node* node, const char* sep, buffer* b);
void tree_print_out(union node*);
void tree_show(union node* node);
#endif

#endif /* TREE_H */
