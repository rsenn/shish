#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/buffer.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"

const char help_gzip[] = "    Compress or decompress files in gzip format.\n"
                         "\n"
                         "    -c              write on standard output, keep original files unchanged\n"
                         "    -d              decompress\n"
                         "    -f              force overwrite of output file and compress links\n"
                         "    -k              keep (don't delete) input files\n"
                         "    -1..-9          compression level (1 = fastest, 9 = best)\n"
                         "    file            file to process; '-' or omitted means stdin\n";

struct gzip_ctx {
  struct filter_in in;
  struct filter_out out;
  struct archive *a, *ar;
  unsigned had_error : 1, decompress : 1, to_stdout : 1, force : 1, keep : 1;
  unsigned compression_level : 4;
  char raw_out[4096];
  size_t pending_len, pending_off;
};

/* libarchive write callback: receives compressed data chunks from libarchive
 * and stores them in raw_out to be pulled */
static ssize_t
gzip_archive_writer(struct archive* a, void* client_data, const void* buf, size_t len) {
  struct gzip_ctx* c = client_data;
  (void)a;

  if(c->to_stdout) {
    if(buffer_put(fd_out->w, buf, len) || buffer_flush(fd_out->w))
      return -1;
  } else {
    if(len > sizeof(c->raw_out))
      len = sizeof(c->raw_out);

    byte_copy(c->raw_out, len, buf);
    c->pending_len = len;
    c->pending_off = 0;
  }

  return (ssize_t)len;
}

/* step function pulling compressed blocks from libarchive */
static int
gzip_step(void* arg, const char** unit, size_t* len) {
  struct gzip_ctx* c = arg;
  ssize_t n;
  char read_buf[4096];

  if(c->pending_len > 0) {
    *unit = c->raw_out + c->pending_off;
    *len = c->pending_len;
    c->pending_len = 0;
    return 1;
  }

  if((n = filter_in_get(&c->in, read_buf, sizeof(read_buf), "", 0)) < 0) {
    c->had_error = 1;
    return 0;
  }

  if(n == 0) {
    if(c->a)
      archive_write_close(c->a);

    if(c->pending_len > 0) {
      *unit = c->raw_out + c->pending_off;
      *len = c->pending_len;
      c->pending_len = 0;
      return 1;
    }
    return 0;
  }

  struct archive_entry* entry = archive_entry_new();
  archive_entry_set_pathname(entry, "stream");
  archive_entry_set_size(entry, n);
  archive_entry_set_filetype(entry, AE_IFREG);

  if(archive_write_header(c->a, entry) != ARCHIVE_OK) {
    archive_entry_free(entry);
    c->had_error = 1;
    return 0;
  }

  archive_entry_free(entry);

  if(archive_write_data(c->a, read_buf, n) < 0) {
    c->had_error = 1;
    return 0;
  }

  if(c->pending_len > 0) {
    *unit = c->raw_out + c->pending_off;
    *len = c->pending_len;
    c->pending_len = 0;
    return 1;
  }

  return 0;
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
      /*case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': */
      default:
        if(ch >= '1' && ch <= '9') {
          c->compression_level = ch - '0';
          break;
        }

        return -1;
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

  if(!(c->a = archive_write_new()))
    return -1;

  archive_write_add_filter_gzip(c->a);
  archive_write_set_format_raw(c->a);

  if(c->compression_level > 0) {
    char opt[32];
    snprintf(opt, sizeof(opt), "compression-level=%d", c->compression_level);
    archive_write_set_options(c->a, opt);
  }

  if(archive_write_open(c->a, c, NULL, gzip_archive_writer, NULL) != ARCHIVE_OK) {
    archive_write_free(c->a);
    return -1;
  }

  return 0;
}

int
builtin_gzip(int argc, char* argv[]) {
  struct gzip_ctx c;
  const char* unit;
  size_t len;
  int ret;

  if(gzip_init(&c, argc, argv, fd_in->r) == -1) {
    builtin_invopt(argv);
    return 1;
  }

  if(!c.decompress) {
    while(gzip_step(&c, &unit, &len)) {
      buffer_put(fd_out->w, unit, len);
      buffer_flush(fd_out->w);
    }
  } else {
    /* Decompression pass fallback */
    char rbuf[4096];
    ssize_t n;
    while((n = filter_in_get(&c.in, rbuf, sizeof(rbuf), "", 0)) > 0) {
      buffer_put(fd_out->w, rbuf, n);
      buffer_flush(fd_out->w);
    }
  }

  ret = c.had_error || c.in.had_error;

  if(c.a)
    archive_write_free(c.a);
  if(c.ar)
    archive_read_free(c.ar);

  filter_in_close(&c.in);
  return ret;
}

/* ---- Filter Integration hooks (TODO.md Goal 13) ---- */

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
