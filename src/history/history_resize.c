#include "shell.h"
#include "scan.h"
#include "history.h"
#include "var.h"

char       **history_array;
unsigned int history_size;

/* resize the command history 
 * ----------------------------------------------------------------------- */
void history_resize(void) {
  unsigned int i;

  /* get history size and return if we already have the desired size */
  unsigned long newsize = DEFAULT_HISTSIZE;
  scan_ulong(var_value("HISTSIZE", NULL), &newsize);

  if(newsize == 0)
    newsize = DEFAULT_HISTSIZE;

  if(++newsize == history_size)
    return;

  /* shrink history if needed */
  for(i = newsize; i < history_size; i++)
    if(history_array[i])
      shell_free(history_array[i]);
 
  history_array = shell_realloc(history_array, newsize * sizeof(char *));

  /* initialize new entries when history has grown */
  for(i = history_size; i < newsize; i++)
    history_array[i] = NULL;

  history_size = newsize;
}

