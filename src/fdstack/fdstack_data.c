#include "../fdstack.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* sends down here-doc data to pipes and reads command expansions from pipes
 * ----------------------------------------------------------------------- */
int
fdstack_data(void) {
  struct fd* fd;
  struct fdstack* st;
  long n;
  char b[FD_BUFSIZE / 2];

  for(st = fdstack; st; st = st->parent) {
    for(fd = st->list; fd; fd = fd->next) {
      /* read from the child and put it into output subst buffer */
      if((fd->mode & FD_WRITE)) {

        while((n = read(fdtable[1]->rb.fd, b, sizeof(b))) > 0) buffer_put(fd->w, b, n);

        buffer_flush(fd->w);
      }

      /* read from the stralloc and put it to here-doc pipe */
      /*    if((fd->mode & FD_HERE) == FD_HERE)
          {
            while((n = buffer_get(&fd->rb, b, sizeof(b))) > 0)
              write(fd->e, b, n);
          }*/
    }
  }

  return 0;
}
