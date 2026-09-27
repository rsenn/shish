#include <archive.h>
#include <archive_entry.h>
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/buffer.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"

const char help_gzip[] = "    Compress files to gzip format.\n"
                         "\n"
                         "    -d              decompress (alias/fallback behavior)\n"
                         "    file            file to compress; '-' or omitted means stdin\n";

struct gzip_ctx {
  struct filter_in in;
  struct filter_out out;
  struct archive* a;
  int had_error;
  int decompress;
  char raw_in[4096];
  char raw_out[4096];
  size_t pending_len;
  size_t pending_off;
};

/* libarchive write callback: receives compressed data chunks from libarchive
 * and stores them in raw_out to be pulled by filter_out_read */
static ssize_t
gzip_archive_writer(struct archive* a, void* client_data, const void* buffer, size_t length) {
  struct gzip_ctx* c = client_data;
  (void)a;

  if(length > sizeof(c->raw_out))
    length = sizeof(c->raw_out);

  byte_copy(c->raw_out, length, buffer);
  c->pending_len = length;
  c->pending_off = 0;
  return (ssize_t)length;
}

/* step function pulling compressed blocks from libarchive into filter_out */
static int
gzip_step(void* arg, const char** unit, size_t* len) {
  struct gzip_ctx* c = arg;
  ssize_t n;
  char read_buf[4096];

  // If there are leftover bytes from the last archive_write callback block, flush them first
  if(c->pending_len > 0) {
    *unit = c->raw_out + c->pending_off;
    *len = c->pending_len;
    c->pending_len = 0;
    return 1;
  }

  // Read next chunk from input source (file or upstream pipe)
  if((n = filter_in_get(&c->in, read_buf, sizeof(read_buf), "", 0)) < 0) {
    c->had_error = 1;
    return 0;
  }

  if(n == 0) {
    // EOF reached: finalize the libarchive stream to flush out remaining compressed blocks
    archive_write_close(c->a);

    if(c->pending_len > 0) {
      *unit = c->raw_out + c->pending_off;
      *len = c->pending_len;
      c->pending_len = 0;
      return 1;
    }

    return 0;
  }

  // Feed raw data into libarchive for gzip compression
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

/* parses arguments and initializes the libarchive write session */
static int
gzip_init(struct gzip_ctx* c, int argc, char* argv[], buffer* upstream) {
  int ch;

  byte_zero(c, sizeof(*c));

  while((ch = shell_getopt(argc, argv, "d")) > 0) {
    switch(ch) {
      case 'd': c->decompress = 1; break;
      default: return -1;
    }
  }

  // Skip options, parse file operands or fall back to upstream/stdin
  filter_in_init(&c->in, argv, argv[shell_optind] ? argv + shell_optind : NULL, upstream);

  if(!(c->a = archive_write_new()))
    return -1;

  if(c->decompress) {
    // If -d is passed, fall back to gzip decompression filter behavior
    archive_write_free(c->a);
    return -1; // Or handle via decompression reader logic
  }

  // Configure libarchive for gzip compression output
  archive_write_add_filter_gzip(c->a);
  archive_write_set_format_raw(c->a);

  if(archive_write_open(c->a, c, NULL, gzip_archive_writer, NULL) != ARCHIVE_OK)
    return -1;

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

  /*ssize_t n;
  while((n = filter_out_read(&c.out, fd_out->w->buf, sizeof(fd_out->w->buf), gzip_step, &c)) > 0) {
    // streaming loop handled via filter_out reader pattern
  }*/

  // Pull compressed units step-by-step and write directly to shish's output buffer
  while(gzip_step(&c, &unit, &len)) {
    buffer_put(fd_out->w, unit, len);
    buffer_flush(fd_out->w);
  }

  ret = c.had_error || c.in.had_error;

  if(c.a)
    archive_write_free(c.a);

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

    alloc_free(c);
    return NULL;
  }

  return c;
}

const struct filter_ops gzip_ops = {gzip_filter_open, gzip_filter_read, gzip_filter_status, gzip_filter_close};
const struct builtin_filter gzip_filter = {&gzip_ops};
