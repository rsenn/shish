#include "buffer.h"
#include "history.h"
#include "parse.h"
#include "sh.h"
#include "str.h"
#include "windoze.h"
#if WINDOWS_NATIVE
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <limits.h>
#endif

buffer history_buffer;
unsigned int history_mapped;
const char history_name[] = "bash_history";
const char* history_files[] = {&history_name[5], /* history */
                               &history_name[2], /* sh_history */
                               &history_name[0], /* bash_history :) */
                               NULL};

/* load the history
 * ----------------------------------------------------------------------- */
void
history_load(void) {
  unsigned int i;
  unsigned int hlen;
  char fname[PATH_MAX + 1];

  hlen = str_copyn(fname, sh_home, PATH_MAX);
  if(hlen >= PATH_MAX - 3)
    return;

  /* append a trailing slash if not already there */
  if(hlen && fname[hlen - 1] != '/')
    fname[hlen++] = '/';

  fname[hlen++] = '.';

  /* mapped history entries */
  history_mapped = 0;

  for(i = 0; history_files[i]; i++) {
    /* append history file name */
    str_copyn(&fname[hlen], history_files[i], PATH_MAX - hlen);

    /* then try to open the history file */
    if(buffer_mmapread(&history_buffer, fname) == 0)
      break;
  }

  /* we tried all files: nothing to load */
  if(history_files[i] == NULL)
    return;

  /* loop through the mapped history file and separate
   * history entries using nul-string-termination
   *
   * note that quote-balancing is applied because there
   * may be quoted multiline words in the history
   */
  while(history_buffer.p < history_buffer.n) {
    unsigned long len;

    /* next entry will begin after the termination */
    if(!(len = history_cmdlen(&history_buffer.x[history_buffer.p])))
      break;

    while(parse_isspace(history_buffer.x[history_buffer.p])) history_buffer.p++;

    history_set(&history_buffer.x[history_buffer.p]);
    history_advance();
    history_mapped++;

    history_buffer.p += len;
  }

  /* unmap the file if it didn't contain any entries */
  if(history_buffer.n && history_mapped == 0)
    buffer_close(&history_buffer);
}
