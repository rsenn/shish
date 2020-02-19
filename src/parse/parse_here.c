#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parses a here-doc body, ending at <delim>
 * ----------------------------------------------------------------------- */
int
parse_here(struct parser* p, stralloc* delim, int nosubst) {
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

    if(r) {
      r = -1;
      break;
    }

    if(p->quot == Q_UNQUOTED) {
      stralloc_catc(&p->sa, (nosubst ? '\'' : '"'));
      continue;
    }

    parse_string(p, 0);

    /* when the parser yields an argstr node
       we have to check for the delimiter */
    if(p->node && p->node->id == N_ARGSTR) {
      unsigned int si, di;
      stralloc* sa;

      sa = &p->node->nargstr.stra;

      /* can't be our delimiter, because we
         do not have the required length yet */
      if(sa->len < delim->len + 1)
        continue;

      si = sa->len;
      di = delim->len;

      if(sa->s[--si] != '\n')
        continue;

      while(sa->s[--si] == delim->s[--di])
        if(di == 0)
          break;

      if(di)
        continue;

      if(si && sa->s[--si] != '\n')
        continue;

      if(si == 0 && p->node != p->tree)
        continue;

      stralloc_trunc(sa, ++si);

      n = tree_newnode(N_ARGSTR);
      n->nargstr.next = p->node;
      p->node = n;

      // parse_string(p, 0);
      break;
    }
  }

  source->mode &= ~SOURCE_HERE;

  return r;
}
