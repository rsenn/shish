#include <shell.h>
#include "history.h"

/* clear a history entry
 * ----------------------------------------------------------------------- */
void history_free(unsigned int index)
{
  /* get the pointer from the specified entry */
  char *s = history_array[index];
  
  /* ..and then clear the entry */
  history_array[index] = NULL;
  
  /* if the string is inside the history_buffer file-mapping we 
   * decrement the count of initial entries still used in the history. 
   * after that, if said count is zero, we'll unmap the file.
   */
  if(history_mapped && s >= history_buffer.x && 
     s < history_buffer.x + history_buffer.n)
  {
    /* if there no mapped entries left we can unmap the file */
    if(--history_mapped == 0)
      buffer_close(&history_buffer);
  }
  /* it was malloced rather than mapped */
  else
  {
    shell_free(s);
  }
}
