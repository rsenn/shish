#include <unistd.h>
#include <limits.h>
#include "str.h"
#include "buffer.h"
#include "stralloc.h"
#include "history.h"
#include "sh.h"

/* save the history
 * ----------------------------------------------------------------------- */
void history_save(void) {
  char data[256];
  buffer b;
  unsigned long hlen;
  char fname[PATH_MAX + 1];

  hlen = str_copyn(fname, sh_home, PATH_MAX);
  if(hlen >= PATH_MAX - 3)
    return;
  
  /* append a trailing slash if not already there */
  if(hlen && fname[hlen - 1] != '/')
    fname[hlen++] = '/';
  
  fname[hlen++] = '.';
  
  /* append history file name */
  str_copyn(&fname[hlen], history_files[0], PATH_MAX - hlen);

  /* unlink the file, so writing doesn't affect previous 
     mappings and lead to a bus error when truncating the file */
  unlink(fname);
  
  /* try to write history */
  if(buffer_truncfile(&b, fname, data, sizeof(data)) == 0) {
    int i;

    for(i = history_size - 1; i >= 0; i--) {
      if(history_array[i]) {
        unsigned long len = history_cmdlen(history_array[i]);
        buffer_put(&b, history_array[i], len);
#ifdef __MINGW32__
        buffer_puts(&b, "\r\n");
#else
        buffer_puts(&b, "\n");
#endif
      }
    }
    
    buffer_flush(&b);
    buffer_close(&b);
  }
}

