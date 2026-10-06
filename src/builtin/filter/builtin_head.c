#include "builtin_config.h"

#if BUILTIN_HEAD

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"

const char help_head[] = "    Copy the first part of files to standard output.\n"
                         "\n"
                         "    -n number       copy the first number lines (default 10)\n"
                         "    -c number       copy the first number bytes instead\n"
                         "                    a negative number copies all but the last number\n"
                         "    -number         same as -n number\n"
                         "    -q              never print the \"==> file <==\" headers\n"
                         "    -v              always print them\n"
                         "    file            file to read; '-' or omitted means stdin\n";

struct head {
  struct filter_in in;
  unsigned long count, left;
  unsigned bytes : 1, quiet : 1, verbose : 1, gotcount : 2, started : 1, neg : 1;
  stralloc out; /* the header of the file being started */
  stralloc all; /* neg: the whole file, until its end is known */
};

static int
head_option(void* ctx, int ch) {
  struct head* h = ctx;

  switch(ch) {
    case 'n':
    case 'c':
      h->bytes = ch == 'c';
      h->gotcount = 1;
      h->neg = shell_optarg[0] == '-';

      if(filter_opt_count(shell_optarg + h->neg, &h->count) < 0) {
        h->in.err_arg = shell_optarg;
        h->in.err_msg = ch == 'c' ? "invalid number of bytes" : "invalid number of lines";
        return -1;
      }

      return 0;

    case 'q': h->quiet = 1; return 0;
    case 'v': h->verbose = 1; return 0;
  }

  /* the old "-5" form: digits build the line count */
  if(ch >= '0' && ch <= '9') {
    h->neg = 0;
    h->count = (h->gotcount == 2 ? h->count * 10 : 0) + (unsigned long)(ch - '0');
    h->bytes = 0;
    h->gotcount = 2;
    return 0;
  }

  return -1;
}

static int
head_setup(void* ctx) {
  struct head* h = ctx;

  if(!h->gotcount)
    h->count = 10;

  return 0;
}

/* whole windows of the input, cut after the last wanted line or byte; the
 * rest of a file is never read. A file gets its header first, when asked for. */
static int
head_step(void* arg, const char** unit, size_t* len) {
  struct head* h = arg;
  const char* p;
  ssize_t n;

  for(;;) {
    size_t take;

    if((n = filter_in_peek(&h->in, &p)) <= 0)
      return 0;

    if(h->in.newfile) {
      int show = h->verbose || (!h->quiet && h->in.files && h->in.files[0] && h->in.files[1]);
      const char* name = filter_in_name(&h->in);

      h->in.newfile = 0;
      h->left = h->count;

      if(show) {
        h->out.len = 0;

        if(h->started)
          stralloc_catc(&h->out, '\n');

        stralloc_cats(&h->out, "==> ");
        stralloc_cats(&h->out, name[0] == '-' && !name[1] ? "standard input" : name);
        stralloc_cats(&h->out, " <==\n");
        h->started = 1;
        *unit = h->out.s;
        *len = h->out.len;
        return 1;
      }

      h->started = 1;
    }

    if(h->neg) {
      /* all but the last count lines/bytes: read to the end of the file, cut there */
      size_t keep;

      h->all.len = 0;

      do {
        stralloc_catb(&h->all, p, (size_t)n);
        filter_in_skip(&h->in, (size_t)n);
      } while((n = filter_in_peek(&h->in, &p)) > 0 && !h->in.newfile);

      keep = h->all.len;

      if(h->bytes) {
        keep = keep > h->count ? keep - h->count : 0;
      } else {
        unsigned long k;

        for(k = h->count; k && keep; k--) {
          size_t end = keep - (h->all.s[keep - 1] == '\n');

          while(end && h->all.s[end - 1] != '\n')
            end--;

          keep = end;
        }
      }

      if(!keep)
        continue;

      *unit = h->all.s;
      *len = keep;
      return 1;
    }

    if(!h->left) {
      filter_in_close(&h->in); /* done with this file: on to the next */
      continue;
    }

    if(h->bytes) {
      take = (size_t)n < h->left ? (size_t)n : h->left;
      h->left -= take;
    } else {
      unsigned long lines = h->left;

      take = (size_t)filter_in_peek_lines(&h->in, &p, &lines);
      h->left = lines;
    }

    filter_in_skip(&h->in, take);
    *unit = p;
    *len = take;
    return 1;
  }
}

static void
head_finish(void* ctx) {
  stralloc_free(&((struct head*)ctx)->out);
  stralloc_free(&((struct head*)ctx)->all);
}

const struct filter_ops head_ops = {
    .opts = "n:c:qv0123456789",
    .size = sizeof(struct head),
    .option = head_option,
    .setup = head_setup,
    .step = head_step,
    .finish = head_finish,
};

FILTER_BUILTIN(head)
#endif /* BUILTIN_HEAD */
