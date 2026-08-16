#include "../expand.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../../lib/byte.h"

/* parses a here-doc body, ending at <delim>
 * ----------------------------------------------------------------------- */
int
parse_here(struct parser* p, stralloc* delim, int nosubst, int strip) {
  int r = 0;
  union node* n;

  /* if there is still a tree from the last call then remove it */
  if(p->tree)
    tree_free(p->tree);

  p->tree = NULL;
  p->node = NULL;

  /* redir_source() reuses this same struct parser across calls when
     several here-docs are queued on one command line (e.g. `cmd <<A
     <<B`); clear leftover text from the previous here-doc's matched
     delimiter line, which the loop below never flushes into p->sa. */
  stralloc_zero(&p->sa);

  /* set the here-doc flag on the source so we won't start
     parsing any other here docs before finishing this one */
  source->mode |= SOURCE_HERE;

  for(;;) {
    /* <<- strips leading tabs from every body line, including the
       closing delimiter's own line (POSIX 2.7.4). Must happen on the
       raw source before parse_squoted/parse_dquoted sees it -- they
       flush p->sa into a tree node as soon as they hit a '$'/'`',
       before control returns here. */
    if(strip) {
      char c;

      while(source_peek(&c) > 0 && c == '\t')
        source_skip();
    }

    p->flags |= P_HERE;

    /* if nosubst is set we treat it like single-quoted otherwise
       like double-quoted, allowing parameter and command expansions */
    r = (nosubst ? parse_squoted : parse_dquoted)(p);
    p->flags &= ~P_HERE;

    /*if(p->quot == Q_UNQUOTED) {
      stralloc_catc(&p->sa, (nosubst ? '\'' : '"'));
      continue;
    }*/

    if(p->sa.len == delim->len + 1) {
      stralloc* sa = &p->sa;

      if(stralloc_endc(sa, '\n'))
        if(byte_equal(sa->s, delim->len, delim->s))
          break;
    }

    parse_string(p, S_HEREDOC);

    if(r)
      break;
  }

  source->mode &= ~SOURCE_HERE;

  return r;
}
