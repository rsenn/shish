#include "../builtin.h"
#include "../fdtable.h"
#include "../term.h"
#include "../../lib/byte.h"
#include "../../lib/open.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
int
filter_open_file(buffer* b, char* rbuf, size_t rlen, const char* path) {
  int fd;

  if(buffer_mmapread(b, path) == 0)
    return 0;

  if((fd = open_read(path)) == -1)
    return -1;

  buffer_init(b, &buffer_op_read, fd, rbuf, rlen);
  return 0;
}

/* ----------------------------------------------------------------------- */
int
filter_copy(const char* path, filter_sink_fn* sink, void* ctx) {
  buffer b;
  char rbuf[4096], chunk[4096];
  ssize_t n;

  if(filter_open_file(&b, rbuf, sizeof(rbuf), path) == -1)
    return -1;

  while((n = buffer_get_until(&b, chunk, sizeof(chunk), "", 0)) > 0)
    sink(ctx, chunk, (size_t)n);

  buffer_close(&b);
  return n < 0 ? -1 : 0;
}

/* ----------------------------------------------------------------------- */
void
filter_in_init(struct filter_in* in, char** errargv, char** files, buffer* upstream) {
  byte_zero(in, sizeof(*in));
  in->errargv = errargv;
  in->files = files;
  in->upstream = upstream;
}

const char*
filter_in_name(const struct filter_in* in) {
  return in->files && in->i > 0 ? in->files[in->i - 1] : "-";
}

void
filter_in_close(struct filter_in* in) {
  if(in->cur && in->cur != in->upstream)
    buffer_close(in->cur);

  in->cur = NULL;
}

/* opens the next operand; 0 when there are none left */
static int
filter_in_next(struct filter_in* in) {
  for(;;) {
    const char* name = "-";

    filter_in_close(in);

    if(in->files) {
      if(!(name = in->files[in->i]))
        return 0;

      in->i++;
    } else if(in->done_any) {
      return 0;
    }

    in->done_any = in->newfile = 1;

    if(!str_diff(name, "-")) {
      in->cur = in->upstream;
      return 1;
    }

    if(filter_open_file(&in->inb, in->rbuf, sizeof(in->rbuf), name) == 0) {
      in->cur = &in->inb;
      return 1;
    }

    builtin_error(in->errargv, (char*)name);
    in->had_error = 1;
  }
}

/* ----------------------------------------------------------------------- */
ssize_t
filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims) {
  for(;;) {
    ssize_t r;

    if(!in->cur && !filter_in_next(in))
      return 0;

    if((r = buffer_get_until(in->cur, buf, len, delims, ndelims)) > 0)
      return r;

    if(r < 0)
      in->had_error = 1;
    else if(in->cur->op == &term_read) {
      buffer_puts(fd_err->w, "EOF");
      buffer_putnlflush(fd_err->w);
    }

    filter_in_close(in);
  }
}

/* ----------------------------------------------------------------------- */
ssize_t
filter_out_read(struct filter_out* out, void* buf, size_t len, filter_step_fn* step, void* ctx) {
  char* p = buf;
  size_t n = 0, take;

  if(out->len) {
    take = out->len < len ? out->len : len;
    byte_copy(p, take, out->pend + out->off);
    out->off += take;
    out->len -= take;
    n = take;
  }

  while(n < len) {
    const char* unit;
    size_t ul;

    if(!step(ctx, &unit, &ul))
      break;

    take = ul < len - n ? ul : len - n;
    byte_copy(p + n, take, unit);
    n += take;

    if(take < ul) {
      out->len = ul - take;
      out->off = 0;
      byte_copy(out->pend, out->len, unit + take);
      break;
    }
  }

  return (ssize_t)n;
}
