#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../parse.h"
#include "../fd.h"
#include "../tree.h"
#include "../source.h"
#include "../debug.h"

/* get a token, the argument indicates whether to search for keywords or not
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_gettok(struct parser* p, int tempflags) {
#ifdef DEBUG_OUTPUT
  size_t start = source->b->p;
#endif
  int oldflags = p->flags;
  p->flags |= tempflags;

  if(!p->pushback || ((p->flags & P_SKIPNL) && p->tok == T_NL)) {
    p->tok = -1;
    /* skip whitespace */
    p->tok = parse_skipspace(p);
    p->tokstart = source->b->p;

    if(p->tree && p->tree->id == N_ARGSTR)
      stralloc_zero(&p->tree->nargstr.stra);

    stralloc_zero(&p->sa);
    /* check for simple tokens first */
    if(p->tok == -1)
      p->tok = parse_simpletok(p);
    /* and finally for words */
    if(p->tok == -1)
      p->tok = parse_word(p);
    /* if the token is a valid name then it could be a keyword */
    if(p->tok & (/* T_WORD | */ T_NAME) && p->node && p->node->id == N_ARGSTR && !(p->flags & P_NOKEYWD))
      parse_keyword(p);

#ifdef DEBUG_OUTPUT
    if(p->tok != -1) {
      buffer_putulong(buffer_2, source->line + 1);
      buffer_puts(buffer_2, ":");
      buffer_putulong(buffer_2, source->column);
      buffer_puts(buffer_2, ": Got token ");
      if(p->flags) {
        char buf[8];
        buffer_puts(buffer_2, "(");
        if(p->flags & P_BQUOTE)
          buffer_puts(buffer_2, "P_BQUOTE ");
        if(p->flags & P_NOKEYWD)
          buffer_puts(buffer_2, "P_NOKEYWD ");
        if(p->flags & ~(P_BQUOTE | P_NOKEYWD))
          buffer_put(buffer_2, buf, fmt_xlong(buf, p->flags & ~(P_BQUOTE | P_NOKEYWD)));

        buffer_puts(buffer_2, ") ");
      }

      buffer_puts(buffer_2, parse_tokname(p->tok, 0));

      {
        char buf[4];
        size_t i;
        buffer_puts(buffer_2, ": \"");
        for(i = start; i < source->b->n; i++) buffer_put(buffer_2, buf, fmt_escapecharshell(buf, source->b->x[i]));
        buffer_puts(buffer_2, "\"");
      }
      buffer_flush(buffer_2);
      /*
            if(p->tok & (T_ASSIGN | T_WORD | T_NAME)) {
              debug_list(p->tree, -1);
            } else if(p->tok & T_REDIR) {
              debug_ulong(" fd", p->tree->nredir.fdes, -1);
              debug_redir("", p->tree->nredir.flag, -1);
              debug_list(p->tree->nredir.list, -1);
              if(p->tree->nredir.data)
                debug_list(p->tree->nredir.data, -1);
            } else if(p->tok != T_NL && p->tree && p->tree->id >= N_ARGSTR) {
              stralloc* sa = &p->tree->nargstr.stra;
              buffer_puts(buffer_2, " '");
              buffer_putsa(buffer_2, sa);
              buffer_putc(buffer_2, '\'');
            }*/
      buffer_putnlflush(buffer_2);
    }
#endif
  }

  p->flags = oldflags;
  p->pushback = 0;
  return p->tok;
}