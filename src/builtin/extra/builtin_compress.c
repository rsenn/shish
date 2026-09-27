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

const char help_compress[] = "    Compress or decompress files; the format follows the command name\n"
                         "    (gzip, bzip2, lbzip2, lz, xz, zstd; default gzip).\n"
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

/* hands out everything queued; stays valid until the next push */
static int
raw_buf_pull(struct raw_buf* rb, const char** unit, size_t* len) {
  if(rb->off < rb->len) {
    *unit = rb->data + rb->off;
    *len = rb->len - rb->off;
    rb->off = rb->len;
    return 1;
  }

  return 0;
}

/* one compressor: command name, file suffix, libarchive filter */
struct compress_algo {
  const char *name, *suffix;
  int (*add)(struct archive*);
};

static const struct compress_algo compress_algos[] = {
    {"gzip", ".gz", archive_write_add_filter_gzip},
    {"bzip2", ".bz2", archive_write_add_filter_bzip2},
    {"lbzip2", ".bz2", archive_write_add_filter_bzip2},
    {"lz", ".lz", archive_write_add_filter_lzip},
    {"xz", ".xz", archive_write_add_filter_xz},
    {"zstd", ".zst", archive_write_add_filter_zstd},
};

/* by command name; anything unknown compresses as gzip */
static const struct compress_algo*
compress_algo_for(const char* name) {
  size_t i;

  for(i = 0; i < sizeof(compress_algos) / sizeof(compress_algos[0]); i++)
    if(str_equal(name, compress_algos[i].name))
      return &compress_algos[i];

  return &compress_algos[0];
}

struct compress_ctx {
  struct filter_in in;
  const struct compress_algo* algo;
  struct archive* a;
  unsigned had_error : 1, decompress : 1, to_stdout : 1, force : 1, keep : 1, started : 1, finished : 1;
  unsigned compression_level : 4;
  struct raw_buf raw;
};

/* libarchive write callback: queues compressed chunks for compress_step() */
static ssize_t
compress_archive_writer(struct archive* a, void* client_data, const void* buf, size_t len) {
  struct compress_ctx* c = client_data;
  (void)a;

  return raw_buf_push(&c->raw, buf, len) < 0 ? -1 : (ssize_t)len;
}

/* opens the single raw entry all the input is written into */
static int
compress_start(struct archive* a) {
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
compress_step(void* arg, const char** unit, size_t* len) {
  struct compress_ctx* c = arg;

  for(;;) {
    const char* p;
    ssize_t n;

    if(raw_buf_pull(&c->raw, unit, len))
      return 1;

    if(c->finished || c->had_error)
      return 0;

    if(!c->started) {
      if(compress_start(c->a) < 0) {
        c->had_error = 1;
        return 0;
      }

      c->started = 1;
    }

    if((n = filter_in_peek(&c->in, &p)) < 0) {
      c->had_error = 1;
      return 0;
    }

    if(n > 0) {
      int w = archive_write_data(c->a, p, n);

      filter_in_skip(&c->in, n);

      if(w < 0) {
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

/* creates a raw writer for algo that hands every compressed chunk to its
 * output at once; level 0 keeps the algorithm's default. NULL on failure */
static struct archive*
compress_writer_new(const struct compress_algo* algo, unsigned level) {
  struct archive* a;

  if(!(a = archive_write_new()))
    return NULL;

  algo->add(a);
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

static int
compress_option(void* ctx, int ch) {
  struct compress_ctx* c = ctx;

  switch(ch) {
    case 'c': c->to_stdout = 1; return 0;
    case 'd': c->decompress = 1; return 0;
    case 'f': c->force = 1; return 0;
    case 'k': c->keep = 1; return 0;
  }

  if(ch < '1' || ch > '9')
    return -1;

  c->compression_level = ch - '0';
  return 0;
}

/* opens the writer; decompression is uncompress's job, so it is valid
 * but not streamable here (1) */
static int
compress_setup(void* ctx) {
  struct compress_ctx* c = ctx;

  c->algo = compress_algo_for(c->in.errargv[0]);

  if(c->decompress)
    return 1;

  if(!(c->a = compress_writer_new(c->algo, c->compression_level)))
    return -1;

  return archive_write_open(c->a, c, NULL, compress_archive_writer, NULL) == ARCHIVE_OK ? 0 : -1;
}

static int
compress_status(void* ctx) {
  struct compress_ctx* c = ctx;
  return c->had_error || c->in.had_error;
}

static void
compress_finish(void* ctx) {
  struct compress_ctx* c = ctx;

  if(c->a)
    archive_write_free(c->a);

  alloc_free(c->raw.data);
}

const struct filter_ops compress_ops = {.opts = "cdfhk123456789",
                                        .size = sizeof(struct compress_ctx),
                                        .option = compress_option,
                                        .setup = compress_setup,
                                        .step = compress_step,
                                        .status = compress_status,
                                        .finish = compress_finish};
const struct builtin_filter compress_filter = {&compress_ops};

/* compresses src into src + suffix or, with decompress set, src minus the suffix into dst;
 * a failed run removes the partial output and keeps the source. -1 on error */
static int
compress_file(struct compress_ctx* c, char* argv[], const char* src) {
  char dst[512];
  size_t len = str_len(src), sl;
  buffer sb;
  char rbuf[4096];
  struct archive* a = NULL;
  int fd, err = 0;
  ssize_t n = 0;

  sl = str_len(c->algo->suffix);

  if(c->decompress) {
    if(len <= sl || len >= sizeof(dst) || !str_equal(src + len - sl, c->algo->suffix)) {
      builtin_errmsg(argv, (char*)src, "unknown suffix -- ignored");
      return -1;
    }

    byte_copy(dst, len - sl, src);
    dst[len - sl] = '\0';
  } else {
    if(len + sl >= sizeof(dst))
      return -1;

    str_copy(dst, src);
    str_copy(dst + len, c->algo->suffix);
  }

  if(!c->force && access(dst, F_OK) == 0) {
    builtin_errmsg(argv, dst, "already exists");
    return -1;
  }

  if((fd = open_trunc(dst)) == -1) {
    builtin_error(argv, dst);
    return -1;
  }

  if(c->decompress) {
    char* zargv[3] = {argv[0], (char*)src, NULL};
    buffer ob;
    char obuf[4096];

    buffer_init(&ob, &buffer_op_write, fd, obuf, sizeof(obuf));
    err = builtin_uncompress_to(2, zargv, &ob) != 0;
    buffer_flush(&ob);
  } else if(filter_open_file(&sb, rbuf, sizeof(rbuf), src) == -1) {
    builtin_error(argv, (char*)src);
    err = 1;
  } else {
    if(!(a = compress_writer_new(c->algo, c->compression_level)) || archive_write_open_fd(a, fd) != ARCHIVE_OK || compress_start(a) < 0)
      err = 1;

    while(!err && (n = buffer_feed(&sb)) > 0) {
      if(archive_write_data(a, buffer_PEEK(&sb), n) < 0)
        err = 1;

      buffer_SEEK(&sb, n);
    }

    if(n < 0)
      err = 1;

    if(a) {
      if(archive_write_close(a) != ARCHIVE_OK)
        err = 1;
      archive_write_free(a);
    }

    buffer_close(&sb);
  }

  close(fd);

  if(err) {
    unlink(dst);
    return -1;
  }

  if(!c->keep)
    unlink(src);

  return 0;
}

/* stdin -> stdout for a "-" operand, independent of the file operand list */
static int
compress_stdin(struct compress_ctx* c, char* argv[]) {
  struct compress_ctx sc;
  int ret;

  byte_zero(&sc, sizeof(sc));
  filter_in_init(&sc.in, argv, NULL, fd_in->r);

  if(!(sc.a = compress_writer_new(c->algo, c->compression_level)) || archive_write_open(sc.a, &sc, NULL, compress_archive_writer, NULL) != ARCHIVE_OK)
    return -1;

  filter_drain(compress_step, &sc, fd_out->w);

  ret = sc.had_error || sc.in.had_error ? -1 : 0;
  archive_write_free(sc.a);
  alloc_free(sc.raw.data);
  filter_in_close(&sc.in);
  return ret;
}

int
builtin_compress(int argc, char* argv[]) {
  struct compress_ctx c;
  int ret = 0;

  if(filter_init(&compress_ops, &c, argc, argv, fd_in->r) < 0) {
    builtin_invopt(argv);
    compress_finish(&c);
    return 1;
  }

  if(c.decompress && (c.to_stdout || !c.in.files)) {
    char* name = argv[shell_optind - 1];
    filter_in_close(&c.in);

    /* uncompress only reads argv[1..]: give it the operands after the options */
    argv[shell_optind - 1] = argv[0];
    ret = builtin_uncompress(argc - shell_optind + 1, argv + shell_optind - 1);
    argv[shell_optind - 1] = name;
    return ret;
  }

  if(c.to_stdout || !c.in.files) {
    filter_drain(compress_step, &c, fd_out->w);
  } else {
    char** files;

    for(files = c.in.files; *files; files++)
      if((str_equal(*files, "-") ? compress_stdin(&c, argv) : compress_file(&c, argv, *files)) < 0)
        c.had_error = 1;
  }

  ret = compress_status(&c);
  compress_finish(&c);
  filter_in_close(&c.in);
  return ret;
}
