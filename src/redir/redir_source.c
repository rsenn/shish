#include <stdlib.h>
#include "tree.h"
#include "parse.h"
#include "redir.h"
#include "expand.h"

/* process all here-docs
 *
 * nredir->data is set to the next here-doc redirection by redir_addhere()
 * after processing it is set to the content of the here-doc (an narg node)
 * ----------------------------------------------------------------------- */
void redir_source(void) {
  struct parser p;
  stralloc delim;
  int r;

  parse_init(&p, P_HERE);

  stralloc_init(&delim);

  for(; redir_list; redir_list = &redir_list->data->nredir) {
    /* expand the delimiter */
    stralloc_init(&delim);
    expand_catsa((union node *)redir_list, &delim, 0);

    /* when any character of the delimiter has been escaped
       then treat the whole here-doc as non-expanded word */
    r = parse_here(&p, &delim, (redir_list->list->nargstr.flag & S_ESCAPED));

    tree_free(redir_list->list);
    redir_list->list = parse_getarg(&p);

    /* free expanded delimiters */
    stralloc_free(&delim);
  }
}

