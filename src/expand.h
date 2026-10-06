#ifndef EXPAND_H
#define EXPAND_H

#include "../lib/stralloc.h"
#include "features.h"
#include "wordlist.h"

#define IFS_DEFAULT " \t\n"

enum subst_type {
  S_TABLE = 0x0f,
  S_UNQUOTED = 0x00,
  S_DQUOTED = 0x01,
  S_SQUOTED = 0x02,
  S_EXPR = 0x03,

  /* substitution types */
  S_SPECIAL = 0xf0,
  S_ARGC = 0x10,     /* $# */
  S_ARGV = 0x20,     /* $* */
  S_ARGVS = 0x30,    /* $@ */
  S_EXITCODE = 0x40, /* $? */
  S_FLAGS = 0x50,    /* $- */
  S_BGEXCODE = 0x60, /* $! */
  S_ARG = 0x70,      /* $[0-9] */
  S_PID = 0x80,      /* $$ */

  S_VAR = 0x0f00,
  S_DEFAULT = 0x0000,  /* ${parameter:-word} */
  S_ASGNDEF = 0x0100,  /* ${parameter:=word} */
  S_ERRNULL = 0x0200,  /* ${parameter:?[word]} */
  S_ALTERNAT = 0x0300, /* ${parameter:+word} */
  S_RSSFX = 0x0400,    /* ${parameter%word} */
  S_RLSFX = 0x0500,    /* ${parameter%%word} */
  S_RSPFX = 0x0600,    /* ${parameter#word} */
  S_RLPFX = 0x0700,    /* ${parameter##word} */
#if WITH_PARAM_RANGE
  S_RANGE = 0x0800, /* ${parameter:offset:length} */
#endif

  S_STRLEN = 0x1000,
  S_NULL = 0x2000, /* treat set but null as unset (:) */
  S_NOSPLIT = 0x4000,
  S_ESCAPED = 0x8000,
  /* a char within here-doc delim is escaped */
  S_GLOB = 0x10000,
  S_ARITH = 0x20000,
  /* a command substitution written as "`...`" rather than "$(...)" --
     purely for tree_cat()'s re-printing, unrelated to whether the
     substitution's result is quoted. Kept outside the S_TABLE-masked
     quoting nibble so it can't be mistaken for a quoting state. */
  S_BQUOTE = 0x40000,
  /* here-document body chunk: its bytes are final, skip expand_unescape() */
  S_HEREDOC = 0x80000
};

/* expansion modes: X_* bits, see wordlist.h */

union node;
struct narg;

#include "tree.h"

/* state: what an expansion leaves behind for its caller
 * ----------------------------------------------------------------------- */

extern char expand_ifs[4];

/* a word expansion failed ("${x?}", "$x" under set -u): the command does not run, status != 0 */
extern int expand_error;

/* where the fields of an expansion live: a command takes arena_tell() before, arena_rewind() after */
extern arena expand_arena;

/* frontend: expand parse-tree words into a list of fields
 * ----------------------------------------------------------------------- */
int expand_args(union node* args, wordlist* wl, int flags);
int expand_vars(union node* vars, wordlist* wl);

/* frontend: expand one parse-tree word, unsplit, into a caller's stralloc
 * ----------------------------------------------------------------------- */
void expand_str(union node*, stralloc* sa, int flags);
void expand_copysa(union node* node, stralloc* sa, int flags);
void expand_catsa(union node* node, stralloc* sa, int flags);

/* extract: turn an expansion into plain C data (stralloc, char*)
 * ----------------------------------------------------------------------- */
void expand_tosa(union node* node, stralloc* sa);
char* expand_tostr(union node* node, int flags);

/* engine: expand the parts of one word (literal, $param, $(cmd), $((expr))) into a wordlist
 * ----------------------------------------------------------------------- */
void expand_arg(union node* narg, wordlist* wl, int flags);
void expand_param(struct nargparam* param, wordlist* wl, int flags);
void expand_command(struct nargcmd* cmd, wordlist* wl, int flags);
void expand_arith(struct nargarith* arith, wordlist* wl, int flags);

/* unescape in place: "\\x" -> "x" where pred(x)
 * ----------------------------------------------------------------------- */
void expand_unescape(stralloc* sa, int (*pred)(int));

/* arithmetic: evaluate an arithmetic tree to an integer
 * ----------------------------------------------------------------------- */
int expand_arith_expr(union node* expr, int64* r);
int expand_arith_binary(struct narithbinary* expr, int64* r);
int expand_arith_assign(struct narithbinary*, int64*);
int expand_arith_unary(struct narithunary* expr, int64* r);
int expand_arith_ternary(struct narithternary* expr, int64* r);

/* rewrites: brace and tilde expansion, applied to a private copy of a word
 * ----------------------------------------------------------------------- */
union node* expand_brace_args(union node* args);
int expand_brace_needed(union node* arg);
void expand_tilde_word(union node* arg);
void expand_tilde_assign(union node* var);
int expand_tilde_needed(union node* arg);
int expand_tilde_assign_needed(union node* var);
int expand_tilde_lookup(
    const char* text, size_t len, int stop_at_colon, stralloc* home, size_t* prefixlen);

#endif /* EXPAND_H */
