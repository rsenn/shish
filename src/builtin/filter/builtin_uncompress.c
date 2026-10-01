#include <archive.h>
#include <archive_entry.h>
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"
#include "../../../lib/str.h"
#include "../../../lib/sig.h"

const char help_uncompress[] = "    Uncompress and concatenate files to standard output (zcat, bzcat, xzcat,\n"
                               "    zstdcat, lbzcat, lz4cat, lzcat, lzopcat). The format is detected from the\n"
                               "    data, not from the command name.\n"
                               "\n"
                               "    gunzip, unxz and unzstd decompress files in place like gzip -d, xz -d and\n"
                               "    zstd -d: file.gz becomes file, -c writes to standard output, -k keeps the\n"
                               "    original, -f overwrites.\n"
                               "\n"
                               "    -f              copy input that is not compressed unchanged\n"
                               "    file            compressed file to read; '-' or omitted means stdin\n";

/* libarchive runs the lzop program to read .lzo data and waits for it
 * itself; the shell's SIGCHLD handler would reap it first and
 * libarchive would report "Child process exited badly". The signal stays
 * blocked from the first hold to the last release.
 *
 *   int  on  1 to hold, 0 to release; holds nest
 * ----------------------------------------------------------------------- */
void
archive_sigchld_hold(int on) {
  static int depth;

  if(on) {
    if(depth++ == 0)
      sig_block(SIGCHLD);
  } else if(depth > 0 && --depth == 0) {
    sig_unblock(SIGCHLD);
  }
}

struct uncompress_ctx {
  struct filter_in in;
  struct archive* a;
  size_t held; /* input bytes lent to libarchive, consumed on its next read */
  unsigned had_error : 1, force : 1, eof : 1, sigheld : 1;
};

/* libarchive read callback: lends the input buffer in place, no copy */
static ssize_t
uncompress_archive_reader(struct archive* a, void* client_data, const void** block) {
  struct uncompress_ctx* c = client_data;
  ssize_t n;

  (void)a;

  filter_in_skip(&c->in, c->held);
  c->held = 0;

  if((n = filter_in_peek(&c->in, (const char**)block)) > 0)
    c->held = (size_t)n;

  return n;
}

/* step function: hands out libarchive's own decode buffer, valid until the next call */
static int
uncompress_step(void* arg, const char** unit, size_t* len) {
  struct uncompress_ctx* c = arg;
  la_int64_t off;
  int r;

  if(c->eof)
    return 0;

  r = archive_read_data_block(c->a, (const void**)unit, len, &off);

  if(r == ARCHIVE_EOF)
    return 0;

  if(r < ARCHIVE_WARN) {
    c->had_error = 1;
    return 0;
  }

  return 1;
}

static int
uncompress_option(void* ctx, int ch) {
  if(ch != 'f')
    return -1;

  ((struct uncompress_ctx*)ctx)->force = 1;
  return 0;
}

/* opens the libarchive read session. unreadable input is reported through
 * had_error, so the input is never consumed and then refused as a usage error. */
static int
uncompress_setup(void* ctx) {
  struct uncompress_ctx* c = ctx;
  struct archive_entry* entry;
  int r;

  archive_sigchld_hold(1);
  c->sigheld = 1;

  if(!(c->a = archive_read_new()))
    return -1;

  // Enable all compression filters (.gz, .zst, .xz, .lzma, etc.)
  archive_read_support_filter_all(c->a);

  // Use RAW format so libarchive handles the stream as unformatted payload bytes
  archive_read_support_format_raw(c->a);

  // Advance past the raw stream header entry; EOF means empty input
  if((r = archive_read_open(c->a, c, NULL, uncompress_archive_reader, NULL)) == ARCHIVE_OK)
    r = archive_read_next_header(c->a, &entry);

  if(r == ARCHIVE_EOF) {
    c->eof = 1;
  } else if(r != ARCHIVE_OK && r != ARCHIVE_WARN) {
    builtin_errmsg(c->in.errargv, (char*)filter_in_name(&c->in), (char*)archive_error_string(c->a));
    c->had_error = c->eof = 1;
  } else if(!c->force && archive_filter_code(c->a, 0) == ARCHIVE_FILTER_NONE) {
    builtin_errmsg(c->in.errargv, (char*)filter_in_name(&c->in), "not in a compressed format");
    c->had_error = c->eof = 1;
  }

  return 0;
}

static int
uncompress_status(void* ctx) {
  struct uncompress_ctx* c = ctx;
  return c->had_error || c->in.had_error;
}

static void
uncompress_finish(void* ctx) {
  struct uncompress_ctx* c = ctx;

  if(c->a) {
    archive_read_close(c->a);
    archive_read_free(c->a);
  }

  if(c->sigheld)
    archive_sigchld_hold(0);
}

const struct filter_ops uncompress_ops = {
    .opts = "f",
    .size = sizeof(struct uncompress_ctx),
    .option = uncompress_option,
    .setup = uncompress_setup,
    .step = uncompress_step,
    .status = uncompress_status,
    .finish = uncompress_finish,
};
const struct builtin_filter uncompress_filter = {&uncompress_ops};

/* runs one uncompress over argv, writing the uncompressed data to out */
int
builtin_uncompress_to(int argc, char* argv[], buffer* out) {
  /* also called from gzip -d, after its own option parsing */
  shell_optind = 1;
  shell_optofs = 0;
  return filter_run(&uncompress_ops, argc, argv, out);
}

int
builtin_uncompress(int argc, char* argv[]) {
  size_t n = str_len(argv[0]);

  /* zcat, lzcat, ... write to stdout; gunzip, unxz, unzstd work on files */
  if(n < 3 || !str_equal(argv[0] + n - 3, "cat"))
    return builtin_compress(argc, argv);

  return builtin_uncompress_to(argc, argv, fd_out->w);
}
