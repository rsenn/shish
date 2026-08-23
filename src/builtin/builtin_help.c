#include "../builtin.h"
#include "../fdtable.h"
#include "../var.h"
#include "../term.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"

static void
output_synopsis(struct builtin_cmd* b) {
  buffer_puts(fd_out->w, b->name);
  buffer_putspace(fd_out->w);
  buffer_puts(fd_out->w, b->args);
}

/* help built-in
 * ----------------------------------------------------------------------- */
const char help_help[] = "    Show a synopsis of every builtin, or the full help for one.\n"
                         "\n"
                         "    command         builtin to show detailed help for\n";

int
builtin_help(int argc, char* argv[]) {
  size_t i, maxlen = 0, rows, offset;
  unsigned int cols;
  const char* vcols;

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

    output_synopsis(b);
    buffer_putnlflush(fd_out->w);

    buffer_puts(fd_out->w, b->help ? b->help : "    No further help available.\n");
    buffer_flush(fd_out->w);

    return 0;
  }

  for(i = 0; builtin_table[i].name; i++) {
    size_t len = str_len(builtin_table[i].name) + 1 + str_len(builtin_table[i].args);

    if(maxlen < len)
      maxlen = len;
  }

#ifdef HAVE_WINSIZE
  term_winsize();

  if(term_size.ws_col)
    maxlen = (term_size.ws_col / 2) - 1;
  else
#else
  maxlen = (80 / 2) - 1; /* no TIOCGWINSZ: assume a stock 80-column terminal */
#endif

    rows = (i + 1) >> 1;

  for(i = 0; i < rows; i++) {
    struct builtin_cmd* b = &builtin_table[i];
    size_t len = str_len(b->name) + 1 + str_len(b->args);

    output_synopsis(b);
    buffer_putnspace(fd_out->w, maxlen + 1 - len);

    b += rows;

    if(b->name)
      output_synopsis(b);

    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
