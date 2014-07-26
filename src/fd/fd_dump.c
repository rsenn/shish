#ifdef DEBUG
#include <fmt.h>
#include <str.h>
#include "fd.h"
#include "fdstack.h"

static const char *fd_flags[] =
{
  "READ",   "WRITE",  "APPEND", "EXCL",
  "TRUNC",  NULL,     NULL,     NULL,
  "FILE",   "DIR",    "LINK",   "CHAR",
  "BLOCK",  "SOCKET", "PIPE",   "STRALLOC",
  "STRING", "DUP",    "TERM"
};

/* dump out debugging info about an (fd) 
 * ----------------------------------------------------------------------- */
void fd_dump(struct fd *fd)
{
  unsigned long n;
  char numbuf[FMT_LONG];
  int out = 0;
  
  if(fd->name == NULL)
    fd_getname(fd);
  
  /* file descriptor */
  n = fmt_long(numbuf, fd->n);
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putspace(fd_out->w);

  /* name */
  n = fd->name ? str_len(fd->name) : 0;
  if(n > 14)
  {
    buffer_put(fd_out->w, fd->name, 12);
    buffer_puts(fd_out->w, "...");
  }
  else
  {
    buffer_put(fd_out->w, fd->name, n);
    buffer_putnspace(fd_out->w, 15 - n);
  }
  
  /* level */
  n = fmt_long(numbuf, fd->stack->level);
  buffer_putnspace(fd_out->w, 3 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putspace(fd_out->w);
  
  /* buffers */
  buffer_dump(fd_out->w, fd->r);
  buffer_putspace(fd_out->w);
  buffer_dump(fd_out->w, fd->w);
  buffer_putspace(fd_out->w);
  
  /* flags */
  for(n = 0; n < sizeof(fd_flags) / sizeof(fd_flags[0]); n++)
  {
    if(fd->mode & (1 << n))
    {
      if(out++) buffer_putc(fd_out->w, '|');
      buffer_puts(fd_out->w, fd_flags[n]);
    }
  }
  
  buffer_putnlflush(fd_out->w);
}
#endif /* DEBUG */
