#include <archive.h>
#include <archive_entry.h>
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"

const char help_zcat[] = "    Uncompress and concatenate files to standard output.\n"
                         "\n"
                         "    file            compressed file to read; '-' or omitted means stdin\n";

struct zcat_ctx {
  struct filter_in in;
  struct filter_out out;
  struct archive* a;
  int had_error;
  char raw_in[4096];
  char raw_out[4096];
};

/* libarchive read callback bridging shish's unified filter input */
static ssize_t
zcat_archive_reader(struct archive* a, void* client_data, const void** block) {
  struct zcat_ctx* c = client_data;
  ssize_t n;

  (void)a;
  
  if((n = filter_in_get(&c->in, c->raw_in, sizeof(c->raw_in), "", 0)) > 0) 
    *block = c->raw_in;
  
  return n;
}

/* step function pulling uncompressed blocks from libarchive into filter_out */
static int
zcat_step(void* arg, const char** unit, size_t* len) {
  struct zcat_ctx* c = arg;
  ssize_t r;

  // Read data block from the active libarchive stream filter pipeline
  if((r = archive_read_data(c->a, c->raw_out, sizeof(c->raw_out))) < 0) {
    c->had_error = 1;
    return 0;
  }
 
  if(r == 0)
    return 0; // EOF

  *unit = c->raw_out;
  *len = (size_t)r;
  return 1;
}

/* parses arguments and initializes the libarchive read session */
static int
zcat_init(struct zcat_ctx* c, int argc, char* argv[], buffer* upstream) {
  struct archive_entry* entry;

  byte_zero(c, sizeof(*c));
  
  // Skip command name, parse files or fall back to upstream/stdin
  filter_in_init(&c->in, argv, argv[1] ? argv + 1 : NULL, upstream);

  c->a = archive_read_new();
  if(!c->a)
    return -1;

  // Enable all compression filters (.gz, .zst, .xz, .lzma, etc.)
  archive_read_support_filter_all(c->a);
  
  // Use RAW format so libarchive handles the stream as unformatted payload bytes
  archive_read_support_format_raw(c->a);

  if(archive_read_open(c->a, c, NULL, zcat_archive_reader, NULL) != ARCHIVE_OK)
    return -1;

  // Advance past the raw stream header entry
  if(archive_read_next_header(c->a, &entry) != ARCHIVE_OK)
    return -1;

  return 0;
}

int
builtin_zcat(int argc, char* argv[]) {
  struct zcat_ctx c;
  const char* unit;
  size_t len;
  int ret;

  if(zcat_init(&c, argc, argv, fd_in->r) == -1) {
    builtin_invopt(argv);
    return 1;
  }

  while(zcat_step(&c, &unit, &len)) {
    buffer_put(fd_out->w, unit, len);
    buffer_flush(fd_out->w);
  }

  ret = c.had_error || c.in.had_error;
    
  if(c.a) {
    archive_read_close(c.a);
    archive_read_free(c.a);
  }

  filter_in_close(&c.in);
  return ret;
}

/* ---- Filter Integration hooks (TODO.md Goal 13) ---- */

static ssize_t
zcat_filter_read(int fd, void* buf, size_t len, void* arg) {
  (void)fd;
  return filter_out_read(&((struct zcat_ctx*)arg)->out, buf, len, zcat_step, arg);
}

static int
zcat_filter_status(void* arg) {
  struct zcat_ctx* c = arg;
  return c->had_error || c->in.had_error ? 1 : 0;
}

static void
zcat_filter_close(void* arg) {
  struct zcat_ctx* c = arg;
  
  if(c->a) {
    archive_read_close(c->a);
    archive_read_free(c->a);
  }

  filter_in_close(&c->in);
  alloc_free(c);
}

/* open returns NULL on error, triggering shish's transparent fork+pipe fallback */
static void*
zcat_filter_open(int argc, char* argv[], buffer* upstream) {
  struct zcat_ctx* c = alloc(sizeof(*c));

  if(!c)
    return NULL;

  if(zcat_init(c, argc, argv, upstream) == -1) {
    if(c->a) {
      archive_read_close(c->a);
      archive_read_free(c->a);
    }

    alloc_free(c);
    return NULL;
  }

  return c;
}

const struct filter_ops zcat_ops = {
  zcat_filter_open, 
  zcat_filter_read, 
  zcat_filter_status, 
  zcat_filter_close
};
const struct builtin_filter zcat_filter = { &zcat_ops };
