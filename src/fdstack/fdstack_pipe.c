<<<<<<< HEAD
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <io.h>
#else
#include <unistd.h>
#endif
#include "fdstack.h"

/* establishs pipes across parent/child for stralloc fds
 * ----------------------------------------------------------------------- */
int fdstack_pipe(unsigned int n, struct fd *fda)
{
  int e;
  struct fd *fd;
  struct fdstack *st;
  unsigned char *b;
  unsigned int ret = 0;
  
  b = (unsigned char *)(&fda[n]);
  
  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next)
  {
    /* make pipes for command expansion outputs */
    if((fd->mode & FD_SUBST) == FD_SUBST)
    {
//      fd->mode |= FD_READ;

      fd_push(fda, fd->n, FD_WRITE|FD_FLUSH);
      fd_setbuf(fda, b, FD_BUFSIZE / 2);

      e = fd_pipe(fda);
      buffer_init(&fd->rb, read, e, NULL, 0);
      b += FD_BUFSIZE / 2;
      fda++;
    }
  }
    
  return ret;
}
