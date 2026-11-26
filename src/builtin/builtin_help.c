#include "builtin_config.h"

#if BUILTIN_HELP

#include "../builtin.h"
#include "../fdtable.h"
#include "../var.h"
#include "../term.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include "../../lib/str.h"
#include "../../lib/scan.h"

static void
output_synopsis(struct builtin_cmd* b) {
  buffer_puts(fd_out->w, b->name);
  buffer_putspace(fd_out->w);
  buffer_puts(fd_out->w, b->args);
}

/* width of column c (widest synopsis in it + 2) when entries run column-major in `rows` rows */
static size_t
help_colwidth(unsigned c, unsigned rows, unsigned n) {
  size_t w = 0;
  unsigned k;

  for(k = c * rows; k < (c + 1) * rows && k < n; k++) {
    size_t len = str_len(builtin_table[k].name) + 1 + str_len(builtin_table[k].args);

    if(w < len)
      w = len;
  }

  return w + 2;
}

/* help built-in
 * ----------------------------------------------------------------------- */
const char help_help[] = "    Show a synopsis of every builtin, or the full help for one.\n"
                         "\n"
                         "    command         builtin to show detailed help for\n";

int
builtin_help(int argc, char* argv[]) {
  size_t i;
  unsigned int cols;

  /* "help command": look up one builtin by name (regardless of its
     B_SPECIAL/B_DEFAULT/B_EXEC class) and print its full synopsis
     line plus whatever help_<name>[] its implementation defines */
  if(argc > 1) {
    struct builtin_cmd* b = 0;

    for(i = 0; builtin_table[i].name; i++) {
      if(!str_diff(builtin_table[i].name, argv[1])) {
        b = &builtin_table[i];
        break;
      }
    }

    if(!b)
      return builtin_errmsg(argv, argv[1], "no help topic matches");

    builtin_usage(b, fd_out->w);
    return 0;
  }

  for(i = 0; builtin_table[i].name; i++) {}

  unsigned n_entries = i, rows, ncols = 1;

  /* stdout is a terminal: pack the synopses column-major into as many
     columns as fit the width; otherwise one builtin per line */
  if(isatty(fd_out->e)) {
    unsigned c;

    cols = 80; /* no TIOCGWINSZ: assume a stock 80-column terminal */

#ifdef HAVE_WINSIZE
    {
      struct winsize sz;

      if(ioctl(fd_out->e, TIOCGWINSZ, &sz) == 0 && sz.ws_col)
        cols = sz.ws_col;
    }
#endif

    /* most columns whose summed widths (widest entry + 2 gap) fit */
    for(ncols = n_entries; ncols > 1; ncols--) {
      size_t total = 0;

      rows = (n_entries + ncols - 1) / ncols;

      for(c = 0; c < ncols; c++)
        total += help_colwidth(c, rows, n_entries);

      if(total - 2 < cols)
        break;
    }
  }

  rows = (n_entries + ncols - 1) / ncols;

  for(i = 0; i < rows; i++) {
    unsigned c;

    for(c = 0; c < ncols; c++) {
      struct builtin_cmd* b;
      unsigned k = c * rows + i;

      if(k >= n_entries)
        break;

      b = &builtin_table[k];
      output_synopsis(b);

      if(c + 1 < ncols && k + rows < n_entries)
        buffer_putnspace(fd_out->w, help_colwidth(c, rows, n_entries) - (str_len(b->name) + 1 + str_len(b->args)));
    }

    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
#endif /* BUILTIN_HELP */
