#include "../fd.h"
#include "../../lib/alloc.h"

struct fd_filter_state {
  const struct filter_ops* ops;
  void* ctx;
};

static ssize_t
fd_filter_op(int fd, void* buf, size_t len, void* arg) {
  /* arg is the buffer* itself (buffer_feed() passes "b", not
     "b->cookie" -- see stralloc_write()'s identical b->cookie
     indirection in lib/stralloc/stralloc_write.c), so the state lives
     one hop further in than a plain buffer_op_proto callback usually
     expects. */
  buffer* b = arg;
  struct fd_filter_state* st = b->cookie;

  (void)fd;
  return st->ops->read(-1, buf, len, st->ctx);
}

static void
fd_filter_deinit(buffer* b) {
  struct fd_filter_state* st = b->cookie;

  if(st) {
    if(st->ops->close)
      st->ops->close(st->ctx);

    b->cookie = NULL;
    alloc_free(st);
  }

  alloc_free(b->x);
  b->x = NULL;
  b->a = 0;
}

/* fd_filter: reconfigure (d)'s read side to pull from a chained
 * filter-capable builtin instead of a real fd -- same idea as
 * fd_here()/fd_subst(), but backed by a real read buffer since bytes
 * are produced on demand through ops->read(), not pre-filled. Takes
 * ownership of ctx: ops->close(ctx) runs from buffer_close() (via
 * ->deinit), same lifecycle as every other virtual fd here.
 * ----------------------------------------------------------------------- */
void
fd_filter(struct fd* d, const struct filter_ops* ops, void* ctx) {
  struct fd_filter_state* st = alloc(sizeof(*st));
  char* buf = alloc(FD_BUFSIZE);

  st->ops = ops;
  st->ctx = ctx;

  d->name = "<filter>";
  d->mode = (d->mode & FD_FREE) | FD_FILTER | FD_READ;

  buffer_init(d->r, &fd_filter_op, -1, buf, FD_BUFSIZE);
  d->r->cookie = st;
  d->r->deinit = &fd_filter_deinit;
  d->e = -1;
}
