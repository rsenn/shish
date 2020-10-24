#include "../debug.h"
#include <assert.h>

#if DEBUG_ALLOC

#include "../fd.h"
#include "../../lib/shell.h"

void
debug_error(const char* file, unsigned int line, const char* s) {
  debug_s(file);
  debug_c(':');
  debug_n(line);
  debug_s(": ");
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
