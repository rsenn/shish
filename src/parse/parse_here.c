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

  /* set the here-doc flag on the source so we won't start
     parsing any other here docs before finishing this one */
  source->mode |= SOURCE_HERE;

  for(;;) {
    p->flags |= P_HERE;

    /* if nosubst is set we treat it like single-quoted otherwise
       like double-quoted, allowing parameter and command expansions */
    r = (nosubst ? parse_squoted : parse_dquoted)(p);
    p->flags &= ~P_HERE;

    /*if(p->quot == Q_UNQUOTED) {
      stralloc_catc(&p->sa, (nosubst ? '\'' : '"'));
      continue;
    }*/

    /* <<- strips leading tabs from every body line, including the
       line containing the closing delimiter (POSIX 2.7.4) */
    if(strip && p->sa.len) {
      size_t i = 0;
      while(i < p->sa.len && p->sa.s[i] == '\t')
        i++;
      if(i) {
        byte_copy(p->sa.s, p->sa.len - i, p->sa.s + i);
        p->sa.len -= i;
      }
    }

    if(p->sa.len == delim->len + 1) {
      stralloc* sa = &p->sa;

      if(stralloc_endc(sa, '\n'))
        if(byte_equal(sa->s, delim->len, delim->s))
          break;
    }

    parse_string(p, 0);

    if(r)
      break;
  }

  source->mode &= ~SOURCE_HERE;

  return r;
}
