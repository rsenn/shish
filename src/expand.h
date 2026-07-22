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
  /* was inside the S_TABLE-masked nibble (as 0x04) alongside the
     quoting states above, which it isn't one of: it marks the syntax
     ("`...`" vs "$(...)") a command substitution was written with,
     purely for tree_cat()'s own re-printing, and is unrelated to
     whether the substitution's *result* is quoted. Sharing that
     nibble made an unquoted backquote substitution's flags OR in as
     nonzero under the S_TABLE mask, so expand_arg() mistook it for
     quoted and suppressed its field splitting -- fixes/60 */
  S_BQUOTE = 0x40000,
  /* set on every N_ARGSTR chunk of a here-document body (parse_here.c).
     parse_squoted.c/parse_dquoted.c both skip their usual parse_isesc
     doubling for P_HERE content -- a heredoc body never undergoes
     pathname expansion, so there's nothing to protect a glob-special
     char from -- meaning a heredoc chunk's bytes are already final and
     must not go through the expand_unescape() pass literal chunks
     elsewhere need to undo that doubling. Without this, that pass ran
     anyway (indistinguishable from a real quoted/unquoted literal
     chunk once stored) and quietly collapsed a genuine "\\" in the
     body down to one backslash (heredoc-body-loses-escaping,
     fixes/71). */
  S_HEREDOC = 0x80000
};

/* expansion modes */
#define X_DEFAULT 0x00000000
#define X_NOSPLIT 0x01000000
/* set only on chunks that came straight from source text (N_ARGSTR),
   where parse_squoted/parse_dquoted/parse_unquoted doubled every
   glob-special char (parse_isesc) to protect it through this pipeline.
   Those chunks need exactly one expand_unescape(parse_isesc) pass to
   reveal the real literal text. Parameter/command/arithmetic
   substitution results (expand_param.c et al) never went through that
   doubling -- they're already real bytes -- so unescaping them again
   corrupts any backslash the substituted value genuinely contains
   (dollar-pid-changes-across-fork's sibling bug, fixes/69). */
#define X_LITERAL 0x02000000
#define X_GLOB 0x04000000
#define X_QUOTED 0x08000000
/* set by expand_cat()'s non-splitting (X_NOSPLIT|X_QUOTED) branch on
   every chunk it appends, literal or not: that branch now unescapes a
   literal chunk itself, immediately, before it ever reaches the
   shared accumulator (fixes/70), so by the time expand_args() etc.
   look at the finished node there's nothing left for their own
   whole-buffer expand_unescape() pass to do -- running it anyway would
   unescape already-final bytes a second time. Marks "don't bother",
   not "was literal": a node built entirely from quoted/no-split chunks
   carries this even if none of them needed unescaping. A node that
   mixes a quoted chunk with a still-unprocessed unquoted one (e.g.
   trailing off a splittable word) is a narrower, pre-existing case
   this doesn't fully resolve either way -- see assign-cmdsubst-value-
   loses-escaping in BUGS. */
#define X_UNESCAPED 0x10000000

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
union node** expand_break(union node** out);
union node* expand_cat(const char* b, unsigned int len, union node** nptr, int flags);
void expand_catsa(union node* node, stralloc* sa, int flags);
union node* expand_command(struct nargcmd* cmd, union node** nptr, int flags);
void expand_copysa(union node* node, stralloc* sa, int flags);
union node* expand_getorcreate(union node** out);
union node* expand_glob(union node** nptr, int flags);
union node* expand_param(struct nargparam* param, union node** nptr, int flags);
void expand_tosa(union node* node, stralloc* sa);
void expand_unescape(stralloc* sa, int (*pred)(int));
int expand_vars(union node* vars, union node** nptr);
void expand_str(union node*, stralloc*, int flags);
char* expand_tostr(union node* node, int flags);

#endif /* EXPAND_H */
