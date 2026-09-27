#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/shell.h"
#include "../../lib/open.h"
#include "../../lib/str.h"

/* open a path for reading into buffer b: mmap when possible, else plain
 * read(2) over rbuf (fifos and devices cannot be mapped). 0 on success, -1 on error.
 * ----------------------------------------------------------------------- */
int
filter_open_file(buffer* b, char* rbuf, size_t rlen, const char* path) {
  int fd;

  /* try memory-mapping the file first for maximum throughput */
  if(buffer_mmapread(b, path) == 0)
    return 0;

  /* fall back to standard read descriptor if mmap is unavailable */
  if((fd = open_read(path)) == -1)
    return -1;

  buffer_init(b, &buffer_op_read, fd, rbuf, rlen);
  return 0;
}

/* feed the whole content of path to sink() in place, block by block;
 * returns -1 if it cannot be read or encounters an error during copy.
 * ----------------------------------------------------------------------- */
int
filter_copy(const char* path, filter_sink_fn* sink, void* ctx) {
  buffer b;
  char rbuf[4096];
  ssize_t n;

  if(filter_open_file(&b, rbuf, sizeof(rbuf), path) == -1)
    return -1;

  while((n = buffer_feed(&b)) > 0) {
    sink(ctx, buffer_PEEK(&b), (size_t)n);
    buffer_SEEK(&b, (size_t)n);
  }

  buffer_close(&b);
  return n < 0 ? -1 : 0;
}

/* initialize a filter_in tracking structure with error arguments, file
 * operands list, and fallback upstream input buffer.
 * ----------------------------------------------------------------------- */
void
filter_in_init(struct filter_in* in, char** errargv, char** files, buffer* upstream) {
  byte_zero(in, sizeof(*in));
  in->errargv = errargv;
  in->files = files;
  in->upstream = upstream;
}

/* return the string name of the file operand currently being read,
 * or "-" if reading from standard input/upstream.
 * ----------------------------------------------------------------------- */
const char*
filter_in_name(const struct filter_in* in) {
  return in->files && in->i > 0 ? in->files[in->i - 1] : "-";
}

/* close the current file buffer if it is a dedicated file operand,
 * leaving the external upstream buffer intact.
 * ----------------------------------------------------------------------- */
void
filter_in_close(struct filter_in* in) {
  if(in->cur && in->cur != in->upstream)
    buffer_close(in->cur);

  in->cur = NULL;
}

/* open the next file operand in sequence; returns 0 when there are
 * no operands left to process.
 * ----------------------------------------------------------------------- */
static int
filter_in_next(struct filter_in* in) {
  for(;;) {
    const char* name = "-";

    filter_in_close(in);

    if(in->files) {
      if(!(name = in->files[in->i]))
        return 0; /* no more operands left */

      in->i++;
    } else if(in->done_any) {
      return 0; /* stdin already consumed when no explicit files given */
    }

    in->done_any = in->newfile = 1;

    /* if name is "-", bind to upstream buffer source */
    if(!str_diff(name, "-")) {
      in->cur = in->upstream;
      return 1;
    }

    /* try opening the physical file path */
    if(filter_open_file(&in->inb, in->rbuf, sizeof(in->rbuf), name) == 0) {
      in->cur = &in->inb;
      return 1;
    }

    /* report error if file opening fails and continue to next */
    builtin_error(in->errargv, (char*)name);
    in->had_error = 1;
  }
}

/* make sure the active source has buffered bytes, cycling file operands
 * on eof. returns 0 once every source is exhausted.
 * ----------------------------------------------------------------------- */
static int
filter_in_ready(struct filter_in* in) {
  for(;;) {
    ssize_t r;

    if(!in->cur && !filter_in_next(in))
      return 0;

    if((r = buffer_feed(in->cur)) > 0)
      return 1;

    if(r < 0)
      in->had_error = 1;

    filter_in_close(in);
  }
}

/* copying read: up to len bytes, stopping after a byte from delims.
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims) {
  while(filter_in_ready(in)) {
    ssize_t r = buffer_get_until(in->cur, buf, len, delims, ndelims);

    if(r > 0)
      return r;

    if(r < 0)
      in->had_error = 1;

    filter_in_close(in);
  }

  return 0;
}

/* zero-copy read: expose the active source's buffered bytes in place.
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_peek(struct filter_in* in, const char** p) {
  if(!filter_in_ready(in))
    return 0;

  *p = buffer_PEEK(in->cur);
  return (ssize_t)buffer_LEN(in->cur);
}

/* consume n bytes previously exposed by filter_in_peek() */
void
filter_in_skip(struct filter_in* in, size_t n) {
  if(in->cur)
    buffer_SEEK(in->cur, n);
}

/* run a step function to completion, writing every unit to out. ctx starts
 * with its struct filter_in; output is flushed only when the next read would
 * have to wait (nothing left buffered in the active source), so an mmapped
 * file goes out in full buffers while a slow pipe or a terminal stays live.
 * ----------------------------------------------------------------------- */
void
filter_drain(filter_step_fn* step, void* ctx, buffer* out) {
  const struct filter_in* in = ctx;
  const char* unit;
  size_t len;

  while(step(ctx, &unit, &len)) {
    buffer_put(out, unit, len);

    if(!in->cur || !buffer_LEN(in->cur))
      buffer_flush(out);
  }

  buffer_flush(out);
}

/* ---- generic open/status/close/run behind a declarative filter_ops ---- */

int
filter_init(const struct filter_ops* ops, void* ctx, int argc, char* argv[], buffer* upstream) {
  struct filter_in* in = ctx;
  int ch;

  byte_zero(ctx, ops->size);

  if(ops->opts || ops->option)
    while((ch = shell_getopt(argc, argv, ops->opts ? ops->opts : "")) > 0)
      if(!ops->option || ops->option(ctx, ch) < 0)
        return -1;

  filter_in_init(in, argv, argv[shell_optind] ? argv + shell_optind : NULL, upstream);
  return ops->setup ? ops->setup(ctx) : 0;
}

void*
filter_open(const struct filter_ops* ops, int argc, char* argv[], buffer* upstream) {
  void* ctx;

  if(!(ctx = alloc(ops->size)))
    return NULL;

  if(filter_init(ops, ctx, argc, argv, upstream) != 0) {
    filter_close(ops, ctx);
    return NULL;
  }

  return ctx;
}

int
filter_status(const struct filter_ops* ops, void* ctx) {
  return ops->status ? ops->status(ctx) : ((struct filter_in*)ctx)->had_error;
}

void
filter_close(const struct filter_ops* ops, void* ctx) {
  if(ops->finish)
    ops->finish(ctx);

  filter_in_close(ctx);
  alloc_free(ctx);
}

int
filter_run(const struct filter_ops* ops, int argc, char* argv[], buffer* out) {
  void* ctx = alloc(ops->size);
  int r, ret;

  if(!ctx)
    return 1;

  if((r = filter_init(ops, ctx, argc, argv, fd_in->r)) < 0) {
    builtin_invopt(argv);
    filter_close(ops, ctx);
    return 1;
  }

  filter_drain(ops->step, ctx, out);
  ret = filter_status(ops, ctx);
  filter_close(ops, ctx);
  return ret;
}
