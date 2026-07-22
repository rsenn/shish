#include "../expand.h"
#include "../fd.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* collect a backquoted command substitution's raw source text and
 * consume its closing (unescaped) backquote, honoring POSIX 2.6.3's
 * escaping rule: a backslash keeps its literal meaning inside
 * backquotes except before '$', '`', or '\\', which it un-escapes.
 * Anything else survives verbatim (including a "$(" or another
 * backslash) for the reparse below to make sense of on its own.
 * ----------------------------------------------------------------------- */
static int
parse_bquoted_raw(struct parser* p, stralloc* raw) {
  char c;

  for(;;) {
    if(source_peek(&c) <= 0)
      return -1;

    if(c == '`') {
      parse_skip(p);
      return 0;
    }

    if(c == '\\') {
      char nextc;

      if(source_next(&nextc) <= 0)
        return -1;

      if(nextc != '$' && nextc != '`' && nextc != '\\')
        stralloc_catc(raw, '\\');

      c = nextc;
    }

    stralloc_catc(raw, c);
    parse_skip(p);
  }
}

/* parse backquoted commands
 * ----------------------------------------------------------------------- */
int
parse_bquoted(struct parser* p) {
  char c;
  int bquote;
  union node* cmds;
  struct parser subp;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  if(source_peek(&c) <= 0)
    return -1;

  bquote = c != '(';

  if(!bquote) {
    if(source_next(&c) <= 0)
      return -1;
    /*
        if(c == '(')
          return parse_arith(p);*/

    parse_init(&subp, P_DEFAULT);

    if((cmds = parse_compound_list(&subp, T_RP)) == NULL)
      return -1;
  } else {
    /* a backquoted substitution can't be recursively parsed straight
       off the live source the way "$( )" above is: the same
       character ('`') both opens and closes it, so a *nested*
       backquoted substitution inside it is only distinguishable via
       backslash-escaping (POSIX 2.6.3) -- e.g. `echo \`echo inner\``
       -- which means the whole body has to be collected and
       unescaped first, then reparsed as its own independent script,
       exactly the way this same body would be if it had arrived as a
       top-level "-c" argument or ". "'d file instead. */
    stralloc raw;
    struct source src;
    struct fd fd;

    parse_skip(p);
    stralloc_init(&raw);

    if(parse_bquoted_raw(p, &raw)) {
      stralloc_free(&raw);
      return -1;
    }

    source_buffer(&src, &fd, raw.s, raw.len);
    parse_init(&subp, P_DEFAULT);

    cmds = parse_compound_list(&subp, T_EOF);

    source_popfd(&fd);
    stralloc_free(&raw);

    if(cmds == NULL)
      return -1;
  }

  parse_newnode(p, N_ARGCMD);
  p->node->nargcmd.flag = (bquote ? S_BQUOTE : 0) | p->quot;
  p->node->nargcmd.list = cmds;

  return 0;
}
