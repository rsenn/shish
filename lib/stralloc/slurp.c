#include "../stralloc.h"
#include "../open.h"

/* reads a whole file into sa (replacing its contents) and appends a NUL
 * that sa->len counts; 0, or -1 if the file can't be read */
int
slurp(const char* fn, stralloc* sa) {
  int fd;

  if((fd = open_read(fn)) == -1)
    return -1;

  if(readclose(fd, sa, 4096) < 0) {
    stralloc_free(sa);
    return -1;
  }

  return stralloc_0(sa) ? 0 : -1;
}
