#include "../fdstack.h"
#include "../debug.h"

/* links an fd to the specfied stack level
 * ----------------------------------------------------------------------- */
void
fdstack_link(struct fdstack* st, struct fd* fd) {
  struct fd **link, *next;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK) && !defined(SHPARSE2AST)
  buffer_puts(debug_output, "fdstack_link n=");
  buffer_putlong(debug_output, fd->n);
  buffer_putnlflush(debug_output);
#endif

  for(link = &st->list; (next = *link); link = &(*link)->next) {
    if(fd->n < next->n)
      break;
  }

  fd->link = link;
  fd->next = next;

  *link = fd;

  fd->stack = st;
}
