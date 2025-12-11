#define DEBUG_NOCOLOR 1
#include "../debug.h"

#ifdef DEBUG_ALLOC
#include "../fdtable.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

static unsigned long debug_memory_total;

void
debug_memory(void) {
  struct chunk* ch;
  char buf[FMT_XLONG + FMT_LONG];
  unsigned long n;

  buffer_puts(fd_out->w, "ptr                size    file:line\n");
  buffer_puts(fd_out->w, "-------------------------------------------------------------------\n");

  debug_memory_total = 0;

  for(ch = debug_heap; ch; ch = ch->next) {
    /* ptr */
    buffer_puts(fd_out->w, "*0x");
    n = fmt_xlonglong(buf, (unsigned long)&ch[1]);
    buffer_putnspace(fd_out->w, 16 - n);
    buffer_put(fd_out->w, buf, n);
    buffer_putspace(fd_out->w);

    /* size */
    n = fmt_ulong(buf, ch->size);
    int m = (9 - n + 1) / 2;
    buffer_putnspace(fd_out->w, m);
    buffer_put(fd_out->w, buf, n);
    buffer_putnspace(fd_out->w, (9 - n) - m);

    /* file */
    const char* f = ch->file;

    n = str_len(ch->file);

    for(int i = 0; i < n; i++) {
      if(!str_diffn(f + i, "/shish", 6)) {
        f += i;
        f++;

        while(*f && *f != '/')
          f++;
        f++;
        break;
      }
    }
    buffer_putspace(fd_out->w);
    buffer_puts(fd_out->w, f);
    n = str_len(f);
    // buffer_putnspace(fd_out->w, 36 - n);
    buffer_putc(fd_out->w, ':');

    /* line */
    n = fmt_ulong(buf, ch->line);
    // buffer_putnspace(fd_out->w, 5 - n);
    buffer_put(fd_out->w, buf, n);

    buffer_putnlflush(fd_out->w);

    debug_memory_total += ch->size;
  }

  buffer_puts(fd_out->w, "----------------------------------------------------------------\n");
  buffer_puts(fd_out->w, "total allocated: ");
  n = fmt_ulong(buf, debug_memory_total);
  buffer_put(fd_out->w, buf, n);
  buffer_putnlflush(fd_out->w);
}
#endif /* DEBUG_OUTPUT */
