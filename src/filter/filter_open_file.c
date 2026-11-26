#include "../filter.h"
#include "../../lib/open.h"
#include "../../lib/buffer.h"

/* open a path for reading into buffer b: mmap when possible, else plain
 * read(2) over rbuf (fifos and devices cannot be mapped). 0 on success, -1 on error.
 * ----------------------------------------------------------------------- */
int
filter_open_file(buffer* b, char* rbuf, size_t rlen, const char* path) {
  int fd;

  /* try memory-mapping the file first for maximum throughput */
  if(buffer_mmapread(b, path) == 0) {
    if(b->n)
      return 0;

    /* mapped as empty: a really empty file, or a device, fifo or /proc file that reports no size (/dev/zero);
       read(2) it below, which gives the bytes of the second kind and an end of file for the first */
  }

  /* fall back to standard read descriptor if mmap is unavailable */
  if((fd = open_read(path)) == -1)
    return -1;

  buffer_init(b, &buffer_op_read, fd, rbuf, rlen);
  return 0;
}
