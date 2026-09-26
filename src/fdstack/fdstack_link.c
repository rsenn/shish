#include "../fdstack.h"
#include "../trace.h"
#include "../debug.h"

/* links an fd to the specfied stack level
 * ----------------------------------------------------------------------- */
void
fdstack_link(struct fdstack* st, struct fd* fd) {
  struct fd **link, *next;

  TRACE(TRACE_FDSTACK, "link", trace_int("n", fd->n));

  for(link = &st->list; (next = *link); link = &(*link)->next) {
    if(fd->n < next->n)
      break;
  }

  fd->link = link;
  fd->next = next;

  *link = fd;

  fd->stack = st;
}
