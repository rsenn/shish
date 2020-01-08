#include "debug.h"
#include <assert.h>

#ifdef DEBUG
#include "fd.h"
#include "shell.h"

void
debug_error(const char* file, unsigned int line, const char* s) {
  buffer_puts(fd_err->w, file);
  buffer_putc(fd_err->w, ':');
  buffer_putulong(fd_err->w, line);
  buffer_puts(fd_err->w, ": ");
  shell_error(s);
#ifdef __GNUC__
  __builtin_trap();
#elif defined(__thumb__)
  asm("bkpt");
#elif defined(__arm__)
  asm("und");
#elif defined(__x86_64__) || defined(__i386__)
  asm("int $3");
#else
#warning No int3/brk/trap instruction
  assert(0);
#endif
}
#endif /* DEBUG */
