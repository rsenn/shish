#ifndef EXPAND_H
#define EXPAND_H

#include "../lib/stralloc.h"

#define IFS_DEFAULT " \t\n"

#define S_TABLE 0x0003
#define S_UNQUOTED (0 << 0)
#define S_DQUOTED (1 << 0)
#define S_SQUOTED (2 << 0)
#define S_EXPR (3 << 0)

#define S_BQUOTE (1 << 2)

/* substitution types */
#define S_SPECIAL 0x00f8
#define S_ARGC (1 << 3)     /* $# */
#define S_ARGV (2 << 3)     /* $* */
#define S_ARGVS (3 << 3)    /* $@ */
#define S_EXITCODE (4 << 3) /* $? */
#define S_FLAGS (5 << 3)    /* $- */
#define S_BGEXCODE (6 << 3) /* $! */
#define S_ARG (7 << 3)      /* $[0-9] */
#define S_PID (8 << 3)      /* $$ */

#define S_VAR 0x0f00
#define S_DEFAULT (0 << 8)  /* ${parameter:-word} */
#define S_ASGNDEF (1 << 8)  /* ${parameter:=word} */
#define S_ERRNULL (2 << 8)  /* ${parameter:?[word]} */
#define S_ALTERNAT (3 << 8) /* ${parameter:+word} */
#define S_RSSFX (4 << 8)    /* ${parameter%word} */
#define S_RLSFX (5 << 8)    /* ${parameter%%word} */
#define S_RSPFX (6 << 8)    /* ${parameter#word} */
#define S_RLPFX (7 << 8)    /* ${parameter##word} */

#define S_STRLEN 0x1000
#define S_NULL 0x2000 /* treat set but null as unset (:) */
#define S_NOSPLIT 0x4000
#define S_ESCAPED 0x8000
/* a char within here-doc delim is escaped */
#define S_GLOB 0x10000

#define S_ARITH 0x80000

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
