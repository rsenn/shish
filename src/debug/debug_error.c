#include "../debug.h"
#include <assert.h>

#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../../lib/shell.h"

void
debug_error(const char* file, unsigned int line, const char* s) {
  buffer_puts(buffer_2, file);
  buffer_putc(buffer_2, ':');
  buffer_putulong(buffer_2, line);
  buffer_puts(buffer_2, ": ");
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
