#include <sys/ioctl.h>
#include <termios.h>
#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../term.h"
#include "../../lib/scan.h"

static void
output_synopsis(struct builtin_cmd* b) {
  buffer_puts(fd_out->w, b->name);
  buffer_putspace(fd_out->w);
  buffer_puts(fd_out->w, b->args);
}

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_help(int argc, char* argv[]) {
  size_t i, maxlen = 0, rows, offset;
  unsigned int cols;

  for(i = 0; builtin_table[i].name; i++) {
    size_t len = str_len(builtin_table[i].name) + 1 + str_len(builtin_table[i].args);
    if(maxlen < len)
      maxlen = len;
  }

  rows = (i + 1) >> 1;

  if((cols = term_size.ws_col))
      maxlen = (cols / 2) - 1;

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
