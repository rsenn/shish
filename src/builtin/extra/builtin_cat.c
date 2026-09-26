#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../term.h"
#include "../../../lib/shell.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/open.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_cat[] = "    Concatenate files to standard output.\n"
                        "\n"
                        "    -n              number every output line\n"
                        "    -b              number only non-empty output lines\n"
                        "    file            file to print; '-' or omitted means stdin\n";

int
builtin_cat(int argc, char* argv[]) {
  int c, ret = 0, number_lines = 0, number_nonempty = 0;
  char* arg;
  ssize_t n, line = 1;
  buffer inb, *in;

  /* check options */
  while((c = shell_getopt(argc, argv, "nb")) > 0) {
    switch(c) {
      case 'n': number_lines = 1; break;
      case 'b': number_nonempty = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(argv[shell_optind] == NULL) {
    argv[shell_optind] = "-";
    argc++;
  }

  while((arg = argv[shell_optind])) {
    char buf[1024], rbuf[1024];

    /*buffer_putm_internal(fd_err->w, "File: '", arg, "'\n", 0);
    buffer_flush(fd_err->w);*/

    if(!str_diff(arg, "-")) {
      in = fd_in->r;
    } else {
      in = &inb;

      if(buffer_mmapread(in, arg)) {
        /* mmap() fails (ESPIPE, "Illegal seek") for anything that
           isn't a regular file -- a FIFO/pipe or character device
           given directly as an operand, both completely ordinary
           things to hand "cat", neither actually an error. Fall back
           to a plain read(2)-based buffer over the same path instead
           of giving up (lib/buffer.h's own comment on buffer_init()
           documents this exact fallback). */
        int rfd = open_read(arg);

        if(rfd == -1) {
          builtin_error(argv, arg);
          ret = 1;
          break;
        }

        buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
      }
    }

    for(;;) {
      if((ret = buffer_get_until(in, buf, sizeof(buf), "\r\n", 2)) == 0) {
        if(in->op == &term_read) {
          buffer_puts(fd_err->w, "EOF");
          buffer_putnlflush(fd_err->w);
        }

        goto next;
      }

      /* a negative return is a real read(2) failure (e.g. EBADF off a
         misresolved fd) -- neither of the branches below handles it,
         so without this the loop just called buffer_get_until()
         again forever instead of ever finishing
         (redir-p.tst-hang-on-fd-chain, fixes/88) */
      if(ret < 0) {
        builtin_error(argv, arg);
        ret = 1;
        break;
      }

      if(ret > 0) {
        char eol = buf[ret - 1];

        if(number_lines || (number_nonempty && ret > 1)) {
          char buf[FMT_ULONG];

          if((n = fmt_ulong(buf, line)) < 5)
            buffer_putnspace(fd_out->w, 5 - n);

          buffer_put(fd_out->w, buf, n);
          buffer_putspace(fd_out->w);
        }

        buffer_put(fd_out->w, buf, ret);
        buffer_flush(fd_out->w);

        if(eol == '\n')
          line++;
      }
    }
  next:
    if(++shell_optind == argc)
      break;
  }

  return ret;
}

/* filter mode (TODO.md Goal 13): cat has no aggregate/short-circuit
 * option, so unlike grep every invocation streams -- the only cutoff
 * is a real per-file open/read error, reported via status(), not by
 * declining up front.
 * ----------------------------------------------------------------------- */
struct cat_filter_ctx {
  char** files; /* argv+shell_optind, or NULL: only "-"/upstream */
  int i;
  int done_any;
  int number_lines, number_nonempty;
  ssize_t line;
  buffer* upstream;
  buffer inb;
  char rbuf[1024];
  buffer* cur; /* NULL: need to open the next file */
  int had_error;
  char* const* errargv; /* for builtin_error(), once we've committed to streaming */

  char raw[1024];     /* one input unit, as read */
  char linebuf[1040]; /* raw plus room for an -n/-b number prefix */
  char pend[1200];    /* a formatted line that didn't fit in a previous read() */
  size_t pend_off, pend_len;
};

/* advance ctx->cur to the next operand, or NULL if none are left */
static int
cat_filter_open_file(struct cat_filter_ctx* c) {
  for(;;) {
    const char* name;

    if(c->cur && c->cur != c->upstream)
      buffer_close(c->cur);

    c->cur = NULL;

    if(c->files) {
      name = c->files[c->i];

      if(!name)
        return 0;

      c->i++;
    } else {
      if(c->done_any)
        return 0;

      name = "-";
    }

    c->done_any = 1;

    if(!str_diff(name, "-")) {
      c->cur = c->upstream;
      c->line = 1;
      return 1;
    }

    c->cur = &c->inb;

    if(buffer_mmapread(c->cur, name) == 0) {
      c->line = 1;
      return 1;
    }

    {
      int rfd = open_read(name);

      if(rfd == -1) {
        builtin_error((char**)c->errargv, (char*)name);
        c->had_error = 1;
        c->cur = NULL;
        continue;
      }

      buffer_init(c->cur, &buffer_op_read, rfd, c->rbuf, sizeof(c->rbuf));
      c->line = 1;
      return 1;
    }
  }
}

/* cat_filter_step: pulls one terminated unit (same "\r\n" delimiter
 * set as builtin_cat()'s own loop) across as many files as it takes,
 * formats it (with -n/-b numbering) into c->linebuf, and hands back a
 * pointer/len. Returns 0 once every file (or just upstream) is
 * exhausted.
 * ----------------------------------------------------------------------- */
static int
cat_filter_step(struct cat_filter_ctx* c, const char** sp, size_t* np) {
  for(;;) {
    ssize_t r;

    if(!c->cur && !cat_filter_open_file(c))
      return 0;

    r = buffer_get_until(c->cur, c->raw, sizeof(c->raw), "\r\n", 2);

    if(r < 0) {
      c->had_error = 1;
      c->cur = NULL;
      continue;
    }

    if(r == 0) {
      c->cur = NULL;
      continue;
    }

    if(c->number_lines || (c->number_nonempty && r > 1)) {
      char nbuf[FMT_ULONG];
      ssize_t nn = fmt_ulong(nbuf, (unsigned long)c->line);
      size_t pos = 0;

      if(nn < 5) {
        byte_copy(c->linebuf, (size_t)(5 - nn), "     ");
        pos = (size_t)(5 - nn);
      }

      byte_copy(c->linebuf + pos, (size_t)nn, nbuf);
      pos += (size_t)nn;
      c->linebuf[pos++] = ' ';
      byte_copy(c->linebuf + pos, (size_t)r, c->raw);

      *sp = c->linebuf;
      *np = pos + (size_t)r;
    } else {
      *sp = c->raw;
      *np = (size_t)r;
    }

    if(c->raw[r - 1] == '\n')
      c->line++;

    return 1;
  }
}

static ssize_t
cat_filter_read(int fd, void* buf, size_t len, void* arg) {
  struct cat_filter_ctx* c = arg;
  char* out = buf;
  size_t n = 0;

  (void)fd;

  if(c->pend_len) {
    size_t take = c->pend_len < (len - n) ? c->pend_len : (len - n);

    byte_copy(out + n, take, c->pend + c->pend_off);
    c->pend_off += take;
    c->pend_len -= take;
    n += take;
  }

  while(n < len) {
    const char* s;
    size_t sn;

    if(!cat_filter_step(c, &s, &sn))
      break;

    if(sn <= len - n) {
      byte_copy(out + n, sn, s);
      n += sn;
    } else {
      size_t take = len - n;

      byte_copy(out + n, take, s);
      n += take;

      c->pend_len = sn - take;
      byte_copy(c->pend, c->pend_len, s + take);
      c->pend_off = 0;
      break;
    }
  }

  return (ssize_t)n;
}

static int
cat_filter_status(void* arg) {
  struct cat_filter_ctx* c = arg;

  return c->had_error ? 1 : 0;
}

static void
cat_filter_close(void* arg) {
  struct cat_filter_ctx* c = arg;

  if(c->cur && c->cur != c->upstream)
    buffer_close(c->cur);

  alloc_free(c);
}

static void*
cat_filter_open(int argc, char* argv[], buffer* upstream) {
  int ch, number_lines = 0, number_nonempty = 0;
  struct cat_filter_ctx* c;

  while((ch = shell_getopt(argc, argv, "nb")) > 0) {
    switch(ch) {
      case 'n': number_lines = 1; break;
      case 'b': number_nonempty = 1; break;
      /* bad option: return NULL *without* printing (builtin_invopt()
         stays the fork fallback's job) -- open() deciding not to
         stream must never have a visible side effect, since the
         caller then re-runs the same argv through the real
         builtin_cat(), which reports it itself, once. */
      default: return NULL;
    }
  }

  if(!(c = alloc(sizeof(*c))))
    return NULL;

  byte_zero(c, sizeof(*c));
  c->upstream = upstream;
  c->errargv = argv;
  c->files = (argv[shell_optind] != NULL) ? argv + shell_optind : NULL;
  c->number_lines = number_lines;
  c->number_nonempty = number_nonempty;
  c->line = 1;
  return c;
}

const struct filter_ops cat_ops = {cat_filter_open,
                                   cat_filter_read,
                                   cat_filter_status,
                                   cat_filter_close};
const struct builtin_filter cat_filter = {&cat_ops};
