#include "../windoze.h"
#include "../buffer.h"
#include "../fmt.h"

#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

extern ssize_t buffer_dummyreadmmap();
extern unsigned long stralloc_write();

void
buffer_dump(buffer* out, buffer* b) {
  char xlong[FMT_LONG + FMT_LONG + FMT_LONG];
  unsigned long n;

#ifndef COLOR_DUMP
#define RED ""
#define GREEN ""
#define YELLOW ""
#define CYAN ""
#define MAGENTA ""
#define NONE ""
#else
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"
#define NONE "\033[0m"
#endif

  buffer_puts(out, "[ ");
  buffer_puts(out, YELLOW "p" CYAN "=" MAGENTA);
  buffer_putulong0(out, b->p, 3);
  buffer_puts(out, NONE ", " YELLOW "n" CYAN "=" MAGENTA);
  buffer_putulong0(out, b->n, 3);
  buffer_puts(out, NONE ", " YELLOW "a" CYAN "=" MAGENTA);
  buffer_putulong0(out, b->a, 3);
  buffer_puts(out, NONE ", " YELLOW "x" CYAN "@" YELLOW "p" CYAN "=" NONE);

  if(b->p > 6) {
    n = b->p;
    buffer_puts(out, "...");
  } else
    n = 0;
  buffer_puts(out, "\"");
  //  buffer_puts(out, "...");
  // buffer_put_escaped(out, &b->x[n], 32);
  buffer_puts(out, "\"");

  buffer_puts(out, NONE ", " YELLOW "fd" CYAN "=" NONE);
  /*if(b->op == (void*)stralloc_write) {
    buffer_puts(out, "*sa");
  } else*/
  {
    n = fmt_long(xlong, b->fd);
    buffer_put(out, xlong, n);
  }
  buffer_puts(out, ", op=");
  /* buffer_putspace(out); */

  if(b->op == (void*)&read)
    buffer_puts(out, "<read>  ");
  else if(b->op == (void*)&write)
    buffer_puts(out, "<write> ");
  else if(b->op == (void*)&buffer_dummyreadmmap)
    buffer_puts(out, "<mmap>  ");
  /*  else if(b->op == (void*)&stralloc_write)
      buffer_puts(out, "<sa-wr> ");*/
  else if(b->op == (void*)NULL)
    buffer_puts(out, "NULL    ");
  else {
    /* n = fmt_xlong(xlong, (int64)(intptr_t)b->op); */
    buffer_putptr(out, (void*)b->op); /* xlong, n); */
  }
  buffer_puts(out, " ]");
  buffer_putnlflush(out);
}
