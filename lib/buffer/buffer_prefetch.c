#include "../buffer.h"
#include "../byte.h"

ssize_t buffer_dummyreadmmap(int, void* buf, size_t len, void* p);
ssize_t buffer_dummyread_fromstr(int, void* buf, size_t len, void* p);
ssize_t buffer_stubborn_read(buffer_op_proto*, int fd, const void* buf, size_t len, void* ptr);

int
buffer_prefetch(buffer* b, size_t n) {
  if(b->p && b->p + n >= b->a) {
    /* a buffer that will never read any more data from an underlying
       fd (mmap'd, or -- buffer_fromstr()'s -c/eval/here-doc/command-
       substitution case -- backed directly by a plain string) has
       nothing to gain from compacting: there's no later read() that
       would benefit from the freed room at the end. Worse, doing it
       anyway is actively wrong for buffer_fromstr(): unlike mmap
       (read-only, and already exempted below), its b->x is the
       *original* string pointer handed in by the caller -- for `-c`,
       that's literally the argv string holding the whole script.
       byte_copy()'ing it left in place to "compact" mutates that
       string as a side effect of mere lookahead, corrupting whatever
       hasn't been consumed yet. Confirmed via a here-document under
       `-c`: something later reads past this same offset again (the
       delimiter search in parse_here.c does exactly that, needing
       both forward progress and to re-examine what it's already
       scanned) and every byte from the current read position onward
       had already been shifted out from under it, so the here-doc
       body came back as the literal delimiter text instead of the
       real content. */
    if((buffer_op_proto*)b->op == (buffer_op_proto*)(void*)&buffer_dummyreadmmap ||
       (buffer_op_proto*)b->op == (buffer_op_proto*)(void*)&buffer_dummyread_fromstr ||
       b->deinit == (void (*)())&buffer_munmap)
      return b->n - b->p;
    byte_copy(b->x, b->n - b->p, &b->x[b->p]);
    b->n -= b->p;
    b->p = 0;
  }

  if(b->p + n >= b->a)
    n = b->a - b->p;

  if(n == 0)
    return -1;

  while(b->n < b->p + n) {
    int w;

    if((w = buffer_stubborn_read(b->op, b->fd, &b->x[b->n], b->a - b->n, b)) < 0)
      return -1;
    b->n += w;

    if(!w)
      break;
  }
  return b->n - b->p;
}
