#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../parse.h"
#include "../trace.h"
#include "../fd.h"
#include "../tree.h"
#include "../source.h"
#include "../sh.h"
#include "../debug.h"
#include "../expand.h"
#include "../../lib/stralloc.h"
#include "builtin_config.h"

/* the word just scanned is an alias name in command position? then its text
 * replaces it as the input and the word is scanned again. Quoted names
 * ('x', "x", \x) and keywords never get here as a plain unquoted word.
 * ----------------------------------------------------------------------- */
#if BUILTIN_ALIAS
static int
parse_alias_subst(struct parser* p) {
  struct nargstr* str;
  struct alias* a;

  if(!(p->tok & (T_NAME | T_WORD)) || !p->tree || p->tree != p->node || p->tree->id != N_ARGSTR)
    return 0;

  str = &p->tree->nargstr;

  if(str->flag & (S_TABLE | S_ESCAPED) || str->stra.len == 0)
    return 0;

  if(!(a = parse_findalias(p, str->stra.s, str->stra.len)))
    return 0;

  {
    size_t codelen;
    const char* code = alias_code(a, &codelen);

    tree_free(p->tree);
    p->tree = p->node = NULL;
    stralloc_zero(&p->sa);
    source_alias_push(a, code, codelen);
  }

  return 1;
}

#else
#define parse_alias_subst(p) 0
#endif

/* does a command word come next, after this token? (assignments and
 * redirections keep the state they found)
 * ----------------------------------------------------------------------- */
static int
parse_alias_next(enum tok_flag tok, int cur) {
  switch(tok) {
    case T_NL:
    case T_SEMI:
    case T_BGND:
    case T_AND:
    case T_OR:
    case T_PIPE:
    case T_LP:
    case T_RP:
    case T_NOT:
    case T_IF:
    case T_THEN:
    case T_ELIF:
    case T_ELSE:
    case T_WHILE:
    case T_UNTIL:
    case T_DO:
    case T_BEGIN: return 1;
    case T_ASSIGN:
    case T_REDIR: return cur;
    default: return 0;
  }
}

/* get a token, the argument indicates whether to search for keywords or not
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_gettok(struct parser* p, int tempflags) {
  int oldflags = p->flags;
  p->flags |= tempflags;

  if(!p->pushback || ((p->flags & P_SKIPNL) && p->tok == T_NL)) {
    int aliasok = p->alias_ok;

    source_alias_reset();

  rescan:
    p->tok = -1;

    /* skip whitespace */
    p->tokstart = source->position;

    if(p->tree && p->tree->id == N_ARGSTR)
      stralloc_zero(&p->tree->nargstr.stra);

    stralloc_zero(&p->sa);

    /* check for simple tokens first */
    if(p->tok == -1)
      p->tok = parse_simpletok(p);

    /* and then for words */
    if(p->tok == -1)
      p->tok = parse_word(p);
    
    /* if the token is a valid name then it could be a keyword */
    if(p->tok & T_NAME && p->node && p->node->id == N_ARGSTR && !(p->flags & P_NOKEYWD))
      parse_keyword(p);

    /* where only "in"/"do" are reserved words, any other one is a plain word */
    if(p->flags & (P_KWIN | P_KWDO)) {
      int only = ((p->flags & P_KWIN) ? T_IN : 0) | ((p->flags & P_KWDO) ? T_DO : 0);

      if((p->tok & (T_CASE | T_DO | T_DONE | T_ELIF | T_ELSE | T_ESAC | T_FI | T_FOR | T_IF | T_IN | T_THEN | T_UNTIL | T_WHILE | T_BEGIN | T_END | T_NOT)) &&
         !(p->tok & only))
        p->tok = T_NAME;
    }

    /* "alias x='cmd '": the word after the replacement is aliasable too */
    if(alias_scan.blank) {
      aliasok = 1;
      alias_scan.blank = 0;
    }

    if(aliasok && !(p->flags & P_NOALIAS) && parse_alias_subst(p))
      goto rescan;

    p->alias_ok = parse_alias_next(p->tok, p->alias_ok);

    if(p->tok != -1)
      TRACE(TRACE_PARSE, "token", trace_str("tok", parse_tokname(p->tok, 0)), trace_hex("flags", p->flags));
  }

  p->flags = oldflags;
  p->pushback = 0;
  return p->tok;
}
