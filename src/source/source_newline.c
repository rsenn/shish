#include "../redir.h"
#include "../source.h"
#include "../prompt.h"

/* should be called everytime we get a new line
 * ----------------------------------------------------------------------- */
void
source_newline(void) {
#if !defined(SHFORMAT) && !defined(SHPARSE2AST)
  /* process any pending here-docs for this line if we're not in a here-doc
     ourselves */
  if(redir_list && (source->mode & SOURCE_HERE) == 0)
    redir_source();
#endif

  /* advance to next line */
  source->position.line++;
  source->position.column = 1;
  source->position.offset++;

#if !defined(SHFORMAT) // && !defined(SHPARSE2AST)
  if(source->mode & SOURCE_IACTIVE)
    prompt_show();
#endif
}
