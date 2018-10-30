#include "debug.h"

#ifdef DEBUG
#include "fmt.h"
#include "str.h"
#include "fd.h"

static unsigned long debug_memory_total;

void debug_memory(void) {
  struct chunk *ch;
  char buf[FMT_XLONG+FMT_LONG];
  unsigned long n;
  
  buffer_puts(fd_out->w, "ptr       size file                     line\n");
  buffer_puts(fd_out->w, "--------------------------------------------\n");
  
  debug_memory_total = 0;
  
  for(ch = debug_heap; ch; ch = ch->next) {
    /* ptr */
    n = fmt_xlong(buf, (long)&ch[1]);
    buffer_putnspace(fd_out->w, 8 - n);
    buffer_put(fd_out->w, buf, n);
    buffer_putspace(fd_out->w);
    
    /* size */
    n = fmt_ulong(buf, ch->size);
    buffer_putnspace(fd_out->w, 5 - n);
    buffer_put(fd_out->w, buf, n);

    /* file */
    n = str_len(ch->file);
    buffer_putspace(fd_out->w);
    buffer_put(fd_out->w, ch->file, n);
    buffer_putnspace(fd_out->w, 24 - n);
    
    /* line */
    n = fmt_ulong(buf, ch->line);
    buffer_putnspace(fd_out->w, 5 - n);
    buffer_put(fd_out->w, buf, n);

    buffer_putnlflush(fd_out->w);
    
    debug_memory_total += ch->size;
  }
    
  buffer_puts(fd_out->w, "--------------------------------------------\n");
  buffer_puts(fd_out->w, "total allocated: ");
  n = fmt_ulong(buf, debug_memory_total);
  buffer_put(fd_out->w, buf, n);
  buffer_putnlflush(fd_out->w);
}
#endif /* DEBUG */
