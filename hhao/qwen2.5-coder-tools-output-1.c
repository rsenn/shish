#ifndef TREE_H
#define TREE_H

#include "source.h"
#include "../lib/stralloc.h"
#include "../lib/uint8.h"
#include "../lib/uint64.h"
#include <stdlib.h>

#ifndef SHISH_TREE_PACKED
#define SHISH_TREE_PACKED /*__attribute__((packed))*/
#endif

enum kind {
  N_SIMPLECMD,
  N_PIPELINE,
  N_AND,
  N_OR,
  N_NOT,
  N_LIST,
  N_SUBSHELL,
  N_BRACEGROUP,
  N_FOR,
  N_CASE,
  N_CASENODE,
  N_IF,
  N_WHILE,
  N_UNTIL,
  N_FUNCTION,
  N_ARG,
  N_ASSIGN,
  N_REDIR,

  A_NUM,
  A_PAREN,
  A_TERNARY,
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

struct ncmd {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* vars;
  union node* args;
} SHISH_TREE_PACKED;

struct npipe {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  unsigned ncmd;
  union node* cmds;
} SHISH_TREE_PACKED;

struct nnot {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* pipeline;
} SHISH_TREE_PACKED;

struct nandor {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* left;
  union node* right;
} SHISH_TREE_PACKED;

struct ngrp {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* cmds;
} SHISH_TREE_PACKED;

struct nfor {
  enum kind id;
  unsigned bgnd : 1;
  unsigned has_in : 1;
  union node* next;
  union node* rdir;
  union node* cmds;
  union node* args;
  char* varn;
} SHISH_TREE_PACKED;

struct ncase {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* list;
  union node* word;
} SHISH_TREE_PACKED;

struct ncasenode {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* pats;
  union node* cmds;
} SHISH_TREE_PACKED;

struct nif {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* cmd0;
  union node* cmd1;
  union node* test;
} SHISH_TREE_PACKED;

struct nloop {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* cmds;
  union node* test;
} SHISH_TREE_PACKED;

struct nfunc {
  enum kind id;
  unsigned dummy : 1;
  struct nfunc* next;
  char* name;
  union node* body;
  struct location loc;
} SHISH_TREE_PACKED;

struct nlist {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* rdir;
  union node* cmds;
} SHISH_TREE_PACKED;

struct narg {
  enum kind id;
  unsigned flag;
  union node* next;
  stralloc stra;
  union node* list;
} SHISH_TREE_PACKED;

struct nredir {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* word;
  union node* data;
  int fdes;
  struct fd* fd;
} SHISH_TREE_PACKED;

struct nargstr {
  enum kind id;
  unsigned flag;
  union node* next;
  stralloc stra;
  struct location loc;
} SHISH_TREE_PACKED;

struct nargparam {
  enum kind id;
  unsigned flag;
  union node* next;
  char* name;
  union node* word;
  long numb;
  struct location loc;
} SHISH_TREE_PACKED;

struct nargcmd {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* list;
} SHISH_TREE_PACKED;

struct nargarith {
  enum kind id;
  unsigned flag;
  union node* next;
  union node* tree;
} SHISH_TREE_PACKED;

struct narithnum {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  int64 num;
  unsigned base;
} SHISH_TREE_PACKED;

struct narithunary {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* node;
} SHISH_TREE_PACKED;

struct narithbinary {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* left;
  union node* right;
} SHISH_TREE_PACKED;

struct narithternary {
  enum kind id;
  unsigned dummy : 1;
  union node* next;
  union node* cond;
  union node* ontrue;
  union node* onfalse;
} SHISH_TREE_PACKED;

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
  struct narithternary narithternary;
};

typedef union node node_t;

#define tree_link(node, nptr) \
  do { \
    *(nptr) = (node); \
    nptr = &(node)->next; \
  } while(0);

#define tree_unshift(node, nptr) \
  do { \
    (node)->next = *(nptr); \
    (*nptr) = (node); \
  } while(0);

#define tree_move(node, nptr) \
  do { \
    *(nptr) = (node); \
    nptr = &(node)->next; \
    (node) = NULL; \
  } while(0);

#define tree_next(nptr) (&((*(nptr)))->next)
#define tree_skip(nptr) ((nptr) = tree_next(nptr))

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

union node* tree_newnode(enum kind nod);
void tree_delnode(union node* node);
void tree_free(union node* list);
union node* tree_copy(union node* node);
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