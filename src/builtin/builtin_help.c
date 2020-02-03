#include "builtin.h"
#include "fd.h"
#include "str.h"
#include "var.h"
#include "scan.h"

static void
output_synopsis(struct builtin_cmd* b) {
  buffer_puts(fd_out->w, b->name);
  buffer_putspace(fd_out->w);
  buffer_puts(fd_out->w, b->args);
}

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_help(int argc, char** argv) {
  size_t n, i, maxlen = 0, rows, offset;
  unsigned int cols;
  const char* vcols;

  for(n = 0; builtin_table[n].name; n++) {
    size_t len = str_len(builtin_table[n].name) + 1 + str_len(builtin_table[n].args);
    if(maxlen < len)
      maxlen = len;
  }

  rows = (n + 1) >> 1;

  vcols = var_get("COLUMNS", &offset);

  if(scan_uint(&vcols[offset], &cols))
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
