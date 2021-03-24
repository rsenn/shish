#include "../fdstack.h"
#include "../debug.h"

/* links an fd to the specfied stack level
 * ----------------------------------------------------------------------- */
void
fdstack_link(struct fdstack* st, struct fd* fd) {
  struct fd **link, *next;

#ifdef DEBUG_FDSTACK
  buffer_puts(&debug_buffer, "fdstack_link n=");
  buffer_putlong(&debug_buffer, fd->n);
  buffer_putnlflush(&debug_buffer);
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
