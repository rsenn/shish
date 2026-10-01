#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

const char help_tail[] = "    Copy the last part of files to standard output.\n"
                         "\n"
                         "    -n number       the last number lines (default 10); +number: from line number on\n"
                         "    -c number       bytes instead of lines; +number: from byte number on\n"
                         "    -number         same as -n number\n"
                         "    -f              keep reading a file as it grows (one file operand)\n"
                         "    -q              never print the \"==> file <==\" headers\n"
                         "    -v              always print them\n"
                         "    file            file to read; '-' or omitted means stdin\n";

struct tail {
  struct filter_in in;
  unsigned long count;
  unsigned bytes : 1, from_start : 1, follow : 1, quiet : 1, verbose : 1, gotcount : 2;
  unsigned open : 1, started : 1; /* a file is being read; a file has been shown */
  unsigned long skip;             /* +N: lines or bytes still to skip */
  unsigned long nl;               /* newlines in buf */
  stralloc buf, out, head;
};

/* [+|-]digits */
static int
tail_number(struct tail* t, const char* s, int ch) {
  unsigned long v = 0;

  t->from_start = 0;

  if(*s == '+')
    t->from_start = 1;

  if(*s == '+' || *s == '-')
    s++;

  if(filter_opt_count(s, &v) < 0) {
    t->in.err_arg = shell_optarg;
    t->in.err_msg = ch == 'c' ? "invalid number of bytes" : "invalid number of lines";
    return -1;
  }

  t->count = v;
  return 0;
}

static int
tail_option(void* ctx, int ch) {
  struct tail* t = ctx;

  switch(ch) {
    case 'n':
    case 'c':
      t->bytes = ch == 'c';
      t->gotcount = 1;
      return tail_number(t, shell_optarg, ch);

    case 'f': t->follow = 1; return 0;
    case 'q': t->quiet = 1; return 0;
    case 'v': t->verbose = 1; return 0;
  }

  /* the old "-5" form */
  if(ch >= '0' && ch <= '9') {
    t->count = (t->gotcount == 2 ? t->count * 10 : 0) + (unsigned long)(ch - '0');
    t->bytes = 0;
    t->from_start = 0;
    t->gotcount = 2;
    return 0;
  }

  return -1;
}

static int
tail_setup(void* ctx) {
  struct tail* t = ctx;

  if(!t->gotcount)
    t->count = 10;

  /* -f ignores a pipe on standard input; on a file it never ends, so a
     chain cannot take it */
  if(t->follow && !(t->in.files && t->in.files[0]))
    t->follow = 0;

  if(t->follow) {
    if(t->in.files[1]) {
      t->in.err_arg = t->in.files[1];
      t->in.err_msg = "-f takes one file";
      return -1;
    }

    t->in.each = 1;
    return 1;
  }

  return 0;
}

/* drops the oldest data so at most count lines (bytes) stay in buf; final is 1
 * at the end of a file, where an unterminated last line counts as a line */
static void
tail_trim(struct tail* t, int final) {
  size_t drop = 0;

  if(t->bytes) {
    if(t->buf.len > t->count)
      drop = t->buf.len - t->count;
  } else {
    unsigned long lines = t->nl + (final && t->buf.len && t->buf.s[t->buf.len - 1] != '\n' ? 1 : 0);
    unsigned long extra = lines > t->count ? lines - t->count : 0;

    while(extra && drop < t->buf.len) {
      size_t k = byte_chr(t->buf.s + drop, t->buf.len - drop, '\n');

      if(drop + k < t->buf.len) {
        drop += k + 1;
        t->nl--;
      } else {
        drop = t->buf.len;
      }

      extra--;
    }
  }

  if(drop) {
    byte_copyr(t->buf.s, t->buf.len - drop, t->buf.s + drop);
    t->buf.len -= drop;
  }
}

/* adds a chunk of the file, trimming now and then so memory stays near count */
static void
tail_feed(struct tail* t, const char* p, size_t n) {
  size_t i;

  stralloc_catb(&t->buf, p, n);

  for(i = 0; i < n; i++)
    t->nl += p[i] == '\n';

  if(t->bytes ? t->buf.len > 2 * t->count + 4096 : t->nl > 2 * t->count + 64)
    tail_trim(t, 0);
}

/* the header of the next file, when asked for */
static void
tail_header(struct tail* t, const char* name) {
  int show = t->verbose || (!t->quiet && t->in.files && t->in.files[0] && t->in.files[1]);

  t->head.len = 0;

  if(!show)
    return;

  if(t->started)
    stralloc_catc(&t->head, '\n');

  stralloc_cats(&t->head, "==> ");
  stralloc_cats(&t->head, name[0] == '-' && !name[1] ? "standard input" : name);
  stralloc_cats(&t->head, " <==\n");
}

/* the finished file as one unit: its header and what the buffer kept */
static void
tail_done(struct tail* t) {
  tail_trim(t, 1);
  t->out.len = 0;
  stralloc_catb(&t->out, t->head.s, t->head.len);
  stralloc_catb(&t->out, t->buf.s, t->buf.len);
  t->buf.len = 0;
  t->nl = 0;
  t->open = 0;
  t->started = 1;
}

static int
tail_step(void* arg, const char** unit, size_t* len) {
  struct tail* t = arg;
  const char* p;
  ssize_t n;

  for(;;) {
    if((n = filter_in_peek(&t->in, &p)) <= 0) {
      if(!t->open)
        return 0;

      t->open = 0;

      if(t->from_start)
        return 0;

      tail_done(t);
      *unit = t->out.s;
      *len = t->out.len;
      return t->out.len > 0;
    }

    if(t->in.newfile) {
      /* the previous file ends here: show it before the new one starts */
      if(t->open) {
        t->open = 0;

        if(!t->from_start) {
          tail_done(t);

          if(t->out.len > 0) {
            *unit = t->out.s;
            *len = t->out.len;
            return 1;
          }
        }
      }

      t->in.newfile = 0;
      t->open = 1;
      t->skip = t->count > 0 ? t->count - 1 : 0;
      tail_header(t, filter_in_name(&t->in));

      /* +N streams: the header goes out first */
      if(t->from_start && t->head.len) {
        *unit = t->head.s;
        *len = t->head.len;
        t->head.len = 0;
        t->started = 1;
        return 1;
      }

      t->started = 1;
    }

    if(!t->from_start) {
      tail_feed(t, p, (size_t)n);
      filter_in_skip(&t->in, (size_t)n);
      continue;
    }

    /* +N: skip the first N-1 lines (bytes), pass the rest on */
    if(t->skip) {
      size_t take;

      if(t->bytes) {
        take = (size_t)n < t->skip ? (size_t)n : t->skip;
        t->skip -= take;
      } else {
        unsigned long lines = t->skip;

        take = (size_t)filter_in_peek_lines(&t->in, &p, &lines);
        t->skip = lines;
      }

      filter_in_skip(&t->in, take);
      continue;
    }

    filter_in_skip(&t->in, (size_t)n);
    *unit = p;
    *len = (size_t)n;
    return 1;
  }
}

/* -f: the end of the file, then whatever is appended to it */
static int
tail_each(void* ctx, const char* src) {
  struct tail* t = ctx;
  char rbuf[4096];
  int fd = (src && !str_equal(src, "-")) ? open(src, O_RDONLY) : 0;
  ssize_t n;

  if(fd == -1) {
    builtin_error(t->in.errargv, (char*)src);
    return 1;
  }

  t->skip = t->count > 0 ? t->count - 1 : 0;

  while((n = read(fd, rbuf, sizeof(rbuf))) > 0) {
    if(!t->from_start) {
      tail_feed(t, rbuf, (size_t)n);
    } else {
      size_t i = 0;

      while(i < (size_t)n && t->skip) {
        if(t->bytes || rbuf[i] == '\n')
          t->skip--;

        i++;
      }

      buffer_put(t->in.sink, rbuf + i, (size_t)n - i);
    }
  }

  if(!t->from_start) {
    tail_trim(t, 1);
    buffer_put(t->in.sink, t->buf.s, t->buf.len);
  }

  buffer_flush(t->in.sink);

  for(;;) {
    struct timespec ts = {0, 100000000};

    if((n = read(fd, rbuf, sizeof(rbuf))) > 0) {
      buffer_put(t->in.sink, rbuf, (size_t)n);
      buffer_flush(t->in.sink);
    } else if(n < 0) {
      builtin_error(t->in.errargv, (char*)src);
      return 1;
    } else {
      nanosleep(&ts, 0);
    }
  }
}

static void
tail_finish(void* ctx) {
  struct tail* t = ctx;

  stralloc_free(&t->buf);
  stralloc_free(&t->out);
  stralloc_free(&t->head);
}

const struct filter_ops tail_ops = {
    .opts = "n:c:fqv0123456789",
    .size = sizeof(struct tail),
    .option = tail_option,
    .setup = tail_setup,
    .step = tail_step,
    .finish = tail_finish,
    .each = tail_each,
};

FILTER_BUILTIN(tail)
