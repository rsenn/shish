#include "redir.h"
#include "source.h"

/* should be called everytime we get a new line
 * ----------------------------------------------------------------------- */
void source_newline(void)
{
  /* process any pending here-docs for this line if we're not in a here-doc
     ourselves */
  if(redir_list && (source->mode & SOURCE_HERE) == 0)
    redir_source();
  
  /* advance to next line */
  source->line++;
}

