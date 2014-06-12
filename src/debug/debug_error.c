#include "debug.h"

#ifdef DEBUG
#include "shell.h"
#include "fd.h"

void debug_error(const char *file, unsigned int line, const char *s)
{
  buffer_puts(fd_err->w, file);
  buffer_putc(fd_err->w, ':');
  buffer_putulong(fd_err->w, line);
  buffer_puts(fd_err->w, ": ");
  shell_error(s);
  asm("int $3");
}
#endif /* DEBUG */
