#include <unistd.h>
#include <buffer.h>
#include <fmt.h>

#ifdef DEBUG

extern int term_read(int fd, char *buf, unsigned int len);
extern int buffer_dummyreadmmap();
extern int stralloc_write();

void buffer_dump(buffer *out, buffer *b) 
{
  char xlong[FMT_LONG+FMT_LONG+FMT_LONG];
  unsigned long n;

  n = fmt_xlong(xlong, (long)out->x);
  buffer_putc(out, '[');
  
  if(b->op == stralloc_write)
  {
    buffer_puts(out, "*sa");
  }
  else
  {
    n = fmt_long(xlong, b->fd);
    buffer_putnspace(out, 3 - n);
    buffer_put(out, xlong, n);
  }
  
  buffer_putspace(out);
  
  n = fmt_ulong(xlong, b->p);
  xlong[n++] = '/';
  n += fmt_ulong(&xlong[n], b->n);
  xlong[n++] = '/';
  n += fmt_ulong(&xlong[n], b->a);
  buffer_putnspace(out, 20 - n);
  buffer_put(out, xlong, n);  
  
  buffer_putspace(out);
  if(b->op == read)
    buffer_puts(out, "<read>  ");
  else if(b->op == write)
    buffer_puts(out, "<write> ");
  else if(b->op == term_read)
    buffer_puts(out, "<term>  ");
  else if(b->op == buffer_dummyreadmmap)
    buffer_puts(out, "<mmap>  ");
  else if(b->op == stralloc_write)
    buffer_puts(out, "<sa-wr> ");
  else if(b->op == NULL)
    buffer_puts(out, "NULL    ");
  else
  {
    n = fmt_xlong(xlong, (long)b->op);
    buffer_put(out, xlong, n);
  }

  buffer_puts(out, " ]");
}
#endif
