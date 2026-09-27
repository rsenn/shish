#include "../builtin.h"
#include "../fdtable.h"
#include "../term.h"
#include "../../lib/byte.h"
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

/* feed the whole content of path to sink() chunk-by-chunk; returns -1
 * if it cannot be read or encounters an error during copy.
 * ----------------------------------------------------------------------- */
int
filter_copy(const char* path, filter_sink_fn* sink, void* ctx) {
  buffer b;
  char rbuf[4096], chunk[4096];
  ssize_t n;

  if(filter_open_file(&b, rbuf, sizeof(rbuf), path) == -1)
    return -1;

  /* stream content token by token into the sink callback */
  while((n = buffer_get_until(&b, chunk, sizeof(chunk), "", 0)) > 0)
    sink(ctx, chunk, (size_t)n);

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

/* read data from the active input source, automatically cycling through
 * file operands sequentially upon reaching eof.
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims) {
  for(;;) {
    ssize_t r;

    /* ensure an active input buffer is open */
    if(!in->cur && !filter_in_next(in))
      return 0;

    /* pull token/data chunk from current buffer */
    if((r = buffer_get_until(in->cur, buf, len, delims, ndelims)) > 0)
      return r;

    if(r < 0)
      in->had_error = 1;
    else if(in->cur->op == &term_read) {
      buffer_puts(fd_err->w, "eof");
      buffer_putnlflush(fd_err->w);
    }

    /* current operand exhausted; close and loop to next file */
    filter_in_close(in);
  }
}

/* turn "one formatted unit per step" into buffer_op_read calls of any size,
 * caching any overflow of a unit that did not fit into the pending buffer.
 * ----------------------------------------------------------------------- */
ssize_t
filter_out_read(struct filter_out* out, void* buf, size_t len, filter_step_fn* step, void* ctx) {
  char* p = buf;
  size_t n = 0, take;

  /* flush out any leftover bytes from the previous step first */
  if(out->len) {
    take = out->len < len ? out->len : len;
    byte_copy(p, take, out->pend + out->off);
    out->off += take;
    out->len -= take;
    n = take;
  }

  /* fetch new units via step function until target buffer is full */
  while(n < len) {
    const char* unit;
    size_t ul;

    if(!step(ctx, &unit, &ul))
      break;

    take = ul < len - n ? ul : len - n;
    byte_copy(p + n, take, unit);
    n += take;

    /* save overflow if the unit was too large to fit entirely */
    if(take < ul) {
      out->len = ul - take;
      out->off = 0;
      byte_copy(out->pend, out->len, unit + take);
      break;
    }
  }

  return (ssize_t)n;
}
