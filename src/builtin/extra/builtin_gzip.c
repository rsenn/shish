#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/buffer.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"
#include "../../../lib/open.h"

const char help_gzip[] = "    Compress or decompress files in gzip format.\n"
                         "\n"
                         "    -c              write on standard output, keep original files unchanged\n"
                         "    -d              decompress\n"
                         "    -f              force overwrite of output file and compress links\n"
                         "    -k              keep (don't delete) input files\n"
                         "    -1..-9          compression level (1 = fastest, 9 = best)\n"
                         "    file            file to process; '-' or omitted means stdin\n";

/* compressed bytes produced by libarchive, waiting to be pulled */
struct raw_buf {
  char* data;
  size_t len, off, cap;
};

static int
raw_buf_push(struct raw_buf* rb, const void* buf, size_t len) {
  if(rb->off == rb->len)
    rb->off = rb->len = 0;

  if(rb->len + len > rb->cap) {
    size_t cap = rb->cap ? rb->cap : 4096;
    char* p;

    while(cap < rb->len + len)
      cap *= 2;

    if(!(p = alloc_re(rb->data, cap)))
      return -1;

    rb->data = p;
    rb->cap = cap;
  }

  byte_copy(rb->data + rb->len, len, buf);
  rb->len += len;
  return 0;
}

/* hands out at most 1024 bytes: filter_out.pend only holds 1400 */
static int
raw_buf_pull(struct raw_buf* rb, const char** unit, size_t* len) {
  if(rb->off < rb->len) {
    size_t n = rb->len - rb->off;

    *unit = rb->data + rb->off;
    *len = n < 1024 ? n : 1024;
    rb->off += *len;
    return 1;
  }

  return 0;
}

struct gzip_ctx {
  struct filter_in in;
  struct filter_out out;
  struct archive *a, *ar;
  unsigned had_error : 1, decompress : 1, to_stdout : 1, force : 1, keep : 1, started : 1, finished : 1;
  unsigned compression_level : 4;
  struct raw_buf raw;
};

/* libarchive write callback: queues compressed chunks for gzip_step() */
static ssize_t
gzip_archive_writer(struct archive* a, void* client_data, const void* buf, size_t len) {
  struct gzip_ctx* c = client_data;
  (void)a;

  return raw_buf_push(&c->raw, buf, len) < 0 ? -1 : (ssize_t)len;
}

/* opens the single raw entry all the input is written into */
static int
gzip_start(struct archive* a) {
  struct archive_entry* entry = archive_entry_new();
  int r;

  archive_entry_set_pathname(entry, "stream");
  archive_entry_set_filetype(entry, AE_IFREG);
  r = archive_write_header(a, entry);
  archive_entry_free(entry);
  return r == ARCHIVE_OK ? 0 : -1;
}

/* step function pulling compressed blocks from libarchive:
 * feeds input until output is queued; at EOF closes to emit the trailer */
static int
gzip_step(void* arg, const char** unit, size_t* len) {
  struct gzip_ctx* c = arg;
  char read_buf[4096];

  for(;;) {
    ssize_t n;

    if(raw_buf_pull(&c->raw, unit, len))
      return 1;

    if(c->finished || c->had_error)
      return 0;

    if(!c->started) {
      if(gzip_start(c->a) < 0) {
        c->had_error = 1;
        return 0;
      }
      
      c->started = 1;
    }

    if((n = filter_in_get(&c->in, read_buf, sizeof(read_buf), "", 0)) < 0) {
      c->had_error = 1;
      return 0;
    }

    if(n > 0) {
      if(archive_write_data(c->a, read_buf, n) < 0) {
        c->had_error = 1;
        return 0;
      }
    } else {
      if(archive_write_close(c->a) != ARCHIVE_OK)
        c->had_error = 1;
      c->finished = 1;
    }
  }
}

/* creates a raw gzip writer that hands every compressed chunk to its
 * output at once; NULL on failure */
static struct archive*
gzip_writer_new(unsigned level) {
  struct archive* a;

  if(!(a = archive_write_new()))
    return NULL;

  archive_write_add_filter_gzip(a);
  archive_write_set_format_raw(a);
  archive_write_set_bytes_per_block(a, 0);
  archive_write_set_bytes_in_last_block(a, 1);

  if(level > 0) {
    char opt[18 + FMT_ULONG + 1];
    size_t n = str_copy(opt, "compression-level=");
    n += fmt_ulong(&opt[n], level);
    opt[n] = '\0';
    archive_write_set_options(a, opt);
  }

  return a;
}

/* parses arguments and initializes sessions */
static int
gzip_init(struct gzip_ctx* c, int argc, char* argv[], buffer* upstream) {
  int ch;

  byte_zero(c, sizeof(*c));
  c->compression_level = 6; /* default gzip level */

  while((ch = shell_getopt(argc, argv, "cdfhk123456789")) > 0) {
    switch(ch) {
      case 'c': c->to_stdout = 1; break;
      case 'd': c->decompress = 1; break;
      case 'f': c->force = 1; break;
      case 'k': c->keep = 1; break;
      default: {
        if(ch >= '1' && ch <= '9') {
          c->compression_level = ch - '0';
          break;
        }

        return -1;
      }
    }
  }

  filter_in_init(&c->in, argv, argv[shell_optind] ? argv + shell_optind : NULL, upstream);

  if(c->decompress) {
    if(!(c->ar = archive_read_new()))
      return -1;

    archive_read_support_filter_gzip(c->ar);
    archive_read_support_format_raw(c->ar);
    return 0;
  }

  if(!(c->a = gzip_writer_new(c->compression_level)))
    return -1;

  if(archive_write_open(c->a, c, NULL, gzip_archive_writer, NULL) != ARCHIVE_OK) {
    archive_write_free(c->a);
    return -1;
  }

  return 0;
}
int
builtin_gzip(int argc, char* argv[]) {
  struct gzip_ctx c;
  int ret = 0;

  if(gzip_init(&c, argc, argv, fd_in->r) == -1) {
    builtin_invopt(argv);
    return 1;
  }

  /* If writing to stdout or reading from stdin pipeline filter */
  if(c.to_stdout || !c.in.files) {
    if(!c.decompress) {
      size_t n;
      const char* x;

      while(gzip_step(&c, &x, &n)) {
        buffer_put(fd_out->w, x, n);
        buffer_flush(fd_out->w);
      }
    } else {
      char rbuf[4096];
      ssize_t r;

      while((r = filter_in_get(&c.in, rbuf, sizeof(rbuf), "", 0)) > 0) {
        buffer_put(fd_out->w, rbuf, r);
        buffer_flush(fd_out->w);
      }
    }
  } else {
    /* File-by-file processing mode: create individual .gz files for each input argument */
    char** files = c.in.files;
    int i = 0;

    while(files[i]) {
      const char* src_name = files[i];
      size_t len = str_len(src_name);

      /* "-" processes stdin to stdout */
      if(str_equal(src_name, "-")) {
        if(!c.decompress) {
          size_t n;
          const char* x;
          while(gzip_step(&c, &x, &n)) {
            buffer_put(fd_out->w, x, n);
            buffer_flush(fd_out->w);
          }
        }
        i++;
        continue;
      }

      char out_name[512];
      if(len + 3 >= sizeof(out_name)) {
        c.had_error = 1;
        i++;
        continue;
      }

      str_copy(out_name, src_name);
      str_copy(out_name + len, ".gz");

      if(!c.force && access(out_name, F_OK) == 0) {
        builtin_error(argv, (char*)out_name);
        c.had_error = 1;
        i++;
        continue;
      }

      int out_fd = open_trunc(out_name);
      if(out_fd == -1) {
        builtin_error(argv, (char*)out_name);
        c.had_error = 1;
        i++;
        continue;
      }

      /* Open individual source file buffer using shish's filter_open_file */
      buffer src_buf;
      char rbuf[4096];
      if(filter_open_file(&src_buf, rbuf, sizeof(rbuf), src_name) == -1) {
        builtin_error(argv, (char*)src_name);
        close(out_fd);
        c.had_error = 1;
        i++;
        continue;
      }

      /* Create a fresh libarchive write session for this specific output file */
      struct archive* file_a = gzip_writer_new(c.compression_level);

      if(!file_a) {
        buffer_close(&src_buf);
        close(out_fd);
        c.had_error = 1;
        i++;
        continue;
      }

      archive_write_open_fd(file_a, out_fd);

      // Alternatively, pump data:
      ssize_t bytes_read;
      char chunk[4096];

      if(gzip_start(file_a) < 0)
        c.had_error = 1;
      else
        while((bytes_read = buffer_get_until(&src_buf, chunk, sizeof(chunk), "", 0)) > 0)
          if(archive_write_data(file_a, chunk, bytes_read) < 0) {
            c.had_error = 1;
            break;
          }

      if(archive_write_close(file_a) != ARCHIVE_OK)
        c.had_error = 1;

      archive_write_free(file_a);

      buffer_close(&src_buf);
      close(out_fd);

      if(!c.keep)
        unlink(src_name);

      i++;
    }
  }

  ret = c.had_error || c.in.had_error;

  if(c.a)
    archive_write_free(c.a);
  if(c.ar)
    archive_read_free(c.ar);

  alloc_free(c.raw.data);
  filter_in_close(&c.in);
  return ret;
}

/* ---- filter integration hooks (todo.md goal 13) ---- */

static ssize_t
gzip_filter_read(int fd, void* buf, size_t len, void* arg) {
  (void)fd;
  return filter_out_read(&((struct gzip_ctx*)arg)->out, buf, len, gzip_step, arg);
}

static int
gzip_filter_status(void* arg) {
  struct gzip_ctx* c = arg;
  return c->had_error || c->in.had_error ? 1 : 0;
}

static void
gzip_filter_close(void* arg) {
  struct gzip_ctx* c = arg;

  if(c->a)
    archive_write_free(c->a);
  if(c->ar)
    archive_read_free(c->ar);

  alloc_free(c->raw.data);
  filter_in_close(&c->in);
  alloc_free(c);
}

static void*
gzip_filter_open(int argc, char* argv[], buffer* upstream) {
  struct gzip_ctx* c = alloc(sizeof(*c));

  if(!c)
    return NULL;

  if(gzip_init(c, argc, argv, upstream) == -1) {
    if(c->a)
      archive_write_free(c->a);
    if(c->ar)
      archive_read_free(c->ar);

    alloc_free(c);
    return NULL;
  }

  return c;
}

const struct filter_ops gzip_ops = {gzip_filter_open, gzip_filter_read, gzip_filter_status, gzip_filter_close};
const struct builtin_filter gzip_filter = {&gzip_ops};
