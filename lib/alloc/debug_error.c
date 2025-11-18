#define DEBUG_NOCOLOR 1
#include "../../src/debug.h"
#include <assert.h>

#ifdef DEBUG_ALLOC

#include "../shell.h"

void
debug_error(const char* file, unsigned int line, const char* s) {
  buffer_puts(shell_buff, file);
  buffer_putc(shell_buff, ':');
  buffer_putlong(shell_buff, line);
  buffer_puts(shell_buff, ": ");
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
#endif /* DEBUG_OUTPUT */
