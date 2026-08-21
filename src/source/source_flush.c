#include "../fd.h"
#include "../source.h"

/* discard the rest of the line the parser gave up on, so the next
 * command can still be read. Dropping the whole read-ahead buffer
 * instead only looks right for a terminal, where it holds just the
 * line being typed: with input from a file (an interactive shell
 * reading a redirected script) it swallows every command after the
 * bad one too.
 * ----------------------------------------------------------------------- */
void
source_flush(void) {
  buffer* b = source->b;
  size_t i;

  for(i = b->p; i < b->n; i++) {
    if(b->x[i] == '\n') {
      b->p = i + 1;
      return;
    }
  }

  b->p = 0;
  b->n = 0;
}
