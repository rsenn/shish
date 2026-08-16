#ifndef EXPAND_H
#define EXPAND_H

#include "../lib/stralloc.h"
#include "features.h"

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
  /* a chunk of a here-document body: never went through the parser's
     glob-special-char doubling that other literal chunks get (a
     heredoc body never undergoes pathname expansion), so it must skip
     the expand_unescape() pass that undoes that doubling -- its bytes
     are already final. */
  S_HEREDOC = 0x80000
};

/* expansion modes */
#define X_DEFAULT 0x00000000
#define X_NOSPLIT 0x01000000
/* set on chunks straight from source text (N_ARGSTR), whose
   glob-special chars the parser doubled to protect them -- these need
   exactly one expand_unescape(parse_isesc) pass. Substitution results
   never went through that doubling, so they must not be marked with
   this (unescaping them again would corrupt a genuine backslash in
   the value, fixes/69). */
#define X_LITERAL 0x02000000
#define X_GLOB 0x04000000
#define X_QUOTED 0x08000000
/* result is already fully processed (unescaped, if needed) by
   expand_cat()'s non-splitting branch -- skip any later whole-buffer
   expand_unescape() pass over it. */
#define X_UNESCAPED 0x10000000
/* result feeds path_fnmatch() (case patterns, ${var%pattern} etc.)
   instead of being used as a plain string -- keep the parser's
   protective backslash-doubling intact instead of unescaping it, since
   that doubling is path_fnmatch()'s own "literal, not a wildcard"
   escape syntax. */
#define X_PATTERN 0x20000000
/* set around the recursive expand_arg(param->word, ...) call for a
   "${parameter+word}"/"${parameter-word}" construct's word -- makes
   expand_cat() treat word's own literal text as splittable, same as
   any other expansion result, instead of exempting it the way a
   top-level command word's literal text is exempt. */
#define X_SUBWORD 0x00100000
/* set on every field of an unquoted word that field-splitting split
   into 2+ fields (including the first, retroactively). Tells
   expand_argv() to keep an empty field that's one of several real
   fields, while still dropping a word's sole, entirely-empty result. */
#define X_SPLIT 0x40000000
/* internal to expand_cat.c: marks a field already closed (finalized)
   by an earlier expand_cat() call for the same word, so a later call
   continuing that word starts a new field instead of appending to it.
   Absence (flag 0) means "open/reusable", matching a virgin node's
   natural zero-flag state -- so a placeholder node expand_args.c
   pre-creates ahead of the next word reads as directly reusable. */
#define X_CATCLOSED 0x80000000

extern char expand_ifs[4];

union node;
struct narg;

#include "tree.h"

union node* expand_arg(union node* narg, union node** nptr, int flags);
int expand_args(union node* args, union node** nptr, int flags);
int expand_argv(union node* args, char** argv);
int expand_arith_binary(struct narithbinary* expr, int64* r);
int expand_arith_expr(union node* expr, int64* r);
int expand_arith_assign(struct narithbinary*, int64*);
union node* expand_arith(struct nargarith* arith, union node** nptr);
int expand_arith_unary(struct narithunary* expr, int64* r);
int expand_arith_ternary(struct narithternary* expr, int64* r);
union node* expand_brace_args(union node* args);
union node** expand_break(union node** out);
union node* expand_cat(const char* b, unsigned int len, union node** nptr, int flags);
void expand_catsa(union node* node, stralloc* sa, int flags);
union node* expand_command(struct nargcmd* cmd, union node** nptr, int flags);
void expand_copysa(union node* node, stralloc* sa, int flags);
union node* expand_getorcreate(union node** out);
union node* expand_glob(union node** nptr, int flags);
int expand_tilde_lookup(
    const char* text, size_t len, int stop_at_colon, stralloc* home, size_t* prefixlen);
void expand_tilde_word(union node* arg);
void expand_tilde_assign(union node* var);
union node* expand_param(struct nargparam* param, union node** nptr, int flags);
void expand_tosa(union node* node, stralloc* sa);
void expand_unescape(stralloc* sa, int (*pred)(int));
int expand_vars(union node* vars, union node** nptr);
void expand_str(union node*, stralloc*, int flags);
char* expand_tostr(union node* node, int flags);

#endif /* EXPAND_H */
