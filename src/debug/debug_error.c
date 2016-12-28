#include "debug.h"

#ifdef DEBUG
#include "shell.h"
#include "fd.h"

void debug_error(const char *file, unsigned int line, const char *s) {
  buffer_puts(fd_err->w, file);
  buffer_putc(fd_err->w, ':');
  buffer_putulong(fd_err->w, line);
  buffer_puts(fd_err->w, ": ");
  shell_error(s);
#ifdef __thumb__
  asm("bkpt");
#elif defined __arm__
  asm("und");
#elif defined __x86_64__ || defined __i386__
  asm("int $3");
#else
  __builtin_trap()
#endif
}
#endif /* DEBUG */
