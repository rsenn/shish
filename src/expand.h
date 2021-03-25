#ifndef EXPAND_H
#define EXPAND_H

#include "../lib/stralloc.h"

#define IFS_DEFAULT " \t\n"

enum subst_type {
  S_TABLE = 0x0003,
  S_UNQUOTED = 0,
  S_DQUOTED = 1,
  S_SQUOTED = 2,
  S_EXPR = 3,

  S_BQUOTE = 4,

  /* substitution types */
  S_SPECIAL = 0x00f8,
  S_ARGC = 0x08,     /* $# */
  S_ARGV = 0x10,     /* $* */
  S_ARGVS = 0x18,    /* $@ */
  S_EXITCODE = 0x20, /* $? */
  S_FLAGS = 0x28,    /* $- */
  S_BGEXCODE = 0x30, /* $! */
  S_ARG = 0x38,      /* $[0-9] */
  S_PID = 0x40,      /* $$ */

  S_VAR = 0x0f00,
  S_DEFAULT = 0,       /* ${parameter:-word} */
  S_ASGNDEF = 0x0100,  /* ${parameter:=word} */
  S_ERRNULL = 0x0200,  /* ${parameter:?[word]} */
  S_ALTERNAT = 0x0300, /* ${parameter:+word} */
  S_RSSFX = 0x0400,    /* ${parameter%word} */
  S_RLSFX = 0x0500,    /* ${parameter%%word} */
  S_RSPFX = 0x0600,    /* ${parameter#word} */
  S_RLPFX = 0x0700,    /* ${parameter##word} */

  S_RANGE = 0x0800, /* ${parameter:offset:length} */

  S_STRLEN = 0x1000,
  S_NULL = 0x2000, /* treat set but null as unset (:) */
  S_NOSPLIT = 0x4000,
  S_ESCAPED = 0x8000,
  /* a char within here-doc delim is escaped */
  S_GLOB = 0x10000,

  S_ARITH = 0x080000
};

/* expansion modes */
#define X_DEFAULT 0x00
#define X_NOSPLIT 0x01
#define X_ESCAPE 0x02
#define X_GLOB 0x04
#define X_QUOTED 0x08

extern char expand_ifs[4];

union node;
struct narg;

#include "tree.h"

//#define EXPAND_INIT(r, p, f) {(union node*)(r), (p), (f)};
/*
static inline union node**
expand_addnode(union node** nptr) {
  if(*(nptr)) {
    nptr = &(*nptr)->next;
  };
  *nptr = tree_newnode(N_ARG);
  return nptr;
}

*/
//
//#define EXPAND_ADDNODE(nptr) (nptr = expand_addnode(nptr), *nptr)

union node* expand_arg(union node* narg, union node** nptr, int flags);
int expand_args(union node* args, union node** nptr, int flags);
void expand_argv(union node* args, char** argv);
int expand_arith_binary(struct narithbinary* expr, int64* r);
int expand_arith_expr(union node* expr, int64* r);
union node* expand_arith(struct nargarith* arith, union node** nptr);
int expand_arith_unary(struct narithunary* expr, int64* r);
union node** expand_break(union node** out);
union node* expand_cat(const char* b, unsigned int len, union node** nptr, int flags);
void expand_catsa(union node* node, stralloc* sa, int flags);
union node* expand_command(struct nargcmd* cmd, union node** nptr, int flags);
void expand_copysa(union node* node, stralloc* sa, int flags);
void expand_escape(stralloc* sa, const char* b, unsigned int n);
union node* expand_getorcreate(union node** out);
union node* expand_glob(union node** nptr, int flags);
union node* expand_param(struct nargparam* param, union node** nptr, int flags);
void expand_tosa(union node* node, stralloc* sa);
void expand_unescape(stralloc* sa, int (*pred)(int));
int expand_vars(union node* vars, union node** nptr);

#endif /* EXPAND_H */
