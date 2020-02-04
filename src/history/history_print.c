#include "../fd.h"
#include "../../lib/fmt.h"
#include "../history.h"

/* print the history
 * ----------------------------------------------------------------------- */
void
history_print(void) {
  unsigned int i, n;
  char numstr[FMT_ULONG];

  if(history_array == NULL)
    return;

  for(i = history_size - 1; i > 0; i--) {
    unsigned long len;

    if(history_array[i] == NULL)
      continue;

    len = history_cmdlen(history_array[i]);

    n = fmt_ulong(numstr, history_count - i + 1);

    buffer_putnspace(fd_out->w, 5 - n);
    buffer_put(fd_out->w, numstr, n);
    buffer_putnspace(fd_out->w, 2);
    buffer_put(fd_out->w, history_array[i], len);
    buffer_putnlflush(fd_out->w);
  }
}
