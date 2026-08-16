#include "../expand.h"
#include "../parse.h"
#include "../redir.h"
#include "../tree.h"
#include <stdlib.h>

/* POSIX 2.7.4: any quoting in the delimiter word suppresses expansion
 * in the here-doc body. word can be a single node or an N_ARG wrapping
 * a mixed list (e.g. E"O"F or EO\F), so every sub-part needs checking
 * for quotes (S_TABLE) or a backslash escape (S_ESCAPED), not just the
 * top node.
 * ----------------------------------------------------------------------- */
static int
redir_here_quoted(union node* word) {
  union node* part;

  for(part = (word->id == N_ARG) ? word->narg.list : word; part; part = part->next)
    if(part->nargstr.flag & (S_TABLE | S_ESCAPED))
      return 1;

  return 0;
}

/* process all here-docs
 *
 * nredir->data is set to the next here-doc redirection by redir_addhere()
 * after processing it is set to the content of the here-doc (an narg node)
 * ----------------------------------------------------------------------- */
void
redir_source(void) {
  struct parser p;
  stralloc delim;

  parse_init(&p, P_HERE);

  stralloc_init(&delim);

  for(; redir_list; redir_list = redir_list->data ? &redir_list->data->nredir : NULL) {
    /* expand the delimiter */
    stralloc_init(&delim);
    expand_str(redir_list->word, &delim, 0);

    /* when any part of the delimiter was quoted or escaped, treat the
       whole here-doc as a non-expanded word (POSIX 2.7.4) */
    if(parse_here(&p, &delim, redir_here_quoted(redir_list->word), (redir_list->flag & R_STRIP))) {
      parse_error(&p, p.tok);
      break;
    }

    tree_free(redir_list->word);
    redir_list->word = parse_getarg(&p);

    /* free expanded delimiters */
    stralloc_free(&delim);
  }
}
