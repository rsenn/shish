#include "../redir.h"
#include "../source.h"
#include "../prompt.h"

/* should be called everytime we get a new line
 * ----------------------------------------------------------------------- */
void
source_newline(void) {
#ifndef SHFORMAT
  /* process any pending here-docs for this line if we're not in a here-doc
     ourselves */
  if(redir_list && (source->mode & SOURCE_HERE) == 0)
    redir_source();
#endif

  /* advance to next line */
  source->pos.line++;
  source->pos.column = 1;

  //#ifndef SHFORMAT
  //  if(source->mode & SOURCE_IACTIVE)
  //    prompt_show();
  //#endif
}
