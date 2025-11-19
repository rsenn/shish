#include "../../lib/uint64.h"
#include "../fdtable.h"
#include "../../lib/fmt.h"
#include "../var.h"
#include "../debug.h"
#include "../term.h"

/* dump a variable entry
 * ----------------------------------------------------------------------- */
void
var_dump(struct var* var) {
  char numbuf[FMT_XLONG * 2];
  size_t n;

  static const int width = 16;
  /* var struct address */
  /*n = fmt_xlonglong(numbuf, (size_t)var);
  buffer_putnc(fd_out->w, '0', sizeof(var) * 2 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putspace(fd_out->w);

  buffer_puts(fd_out->w, CURSOR_HORIZONTAL_ABSOLUTE(19));*/
  /* variable name */
  if(var->len > width) {
    buffer_put(fd_out->w, var->sa.s, (width - 3));
    buffer_puts(fd_out->w, "...");
  } else {
    buffer_put(fd_out->w, var->sa.s, var->len);
    buffer_putnspace(fd_out->w, width - var->len);
  }

  /* variable value */
  n = var_vlen(var->sa.s);
  buffer_puts(fd_out->w, CURSOR_HORIZONTAL_ABSOLUTE(19));

  if(var->offset == var->len) {
    buffer_putc(fd_out->w, '-');
  } else {
    buffer_putc(fd_out->w, '"');

    if(n) {
      unsigned int i, l, rl, rn;
      rn = var->sa.len - var->offset;
      rl = (rn > width ? (width - 3) : rn);
      l = (n > width ? (width - 3) : n);

      for(i = 0; i < rl; i++) {
        if(var->sa.s[var->offset + i] != '\n' && var->sa.s[var->offset + i] != '\t')
          buffer_put(fd_out->w, &var->sa.s[var->offset + i], 1);
        else
          buffer_putspace(fd_out->w);
      }

      if(rl < rn) {
        buffer_puts(fd_out->w, "...");
        rl += 3;
        l += 3;
      }
      n = width - rl;
    } else
      n = width;
    buffer_putc(fd_out->w, '"');
  }

  buffer_putspace(fd_out->w);
  term_escape(fd_out->w, width * 2, 'G');

  /* name length */
  n = fmt_ulong(numbuf, var->len);
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_put(fd_out->w, numbuf, n);

  /* value offset */
  n = fmt_ulong(numbuf, var->offset);
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_put(fd_out->w, numbuf, n);

  /* total length */
  n = fmt_ulong(numbuf, var->sa.len);
  buffer_putnspace(fd_out->w, 6 - n);
  buffer_put(fd_out->w, numbuf, n);

  /* vartab level */
  n = fmt_ulong(numbuf, var->table->level);
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_put(fd_out->w, numbuf, n);

  /* vartab bucket */
  n = fmt_ulong(numbuf, (var->rndhash % VARTAB_BUCKETS));
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_put(fd_out->w, numbuf, n);

  buffer_putspace(fd_out->w);

  /* lexical hash */
  term_escape(fd_out->w, width * 2 + 26, 'G');
  buffer_putxlonglong0(fd_out->w, var->lexhash, 16);
  buffer_putspace(fd_out->w);

  /* randomized hash */
  term_escape(fd_out->w, width * 2 + 43, 'G');
  buffer_putxlonglong0(fd_out->w, var->rndhash, 16);
  buffer_putspace(fd_out->w);

  /* flags */
  term_escape(fd_out->w, width * 2 + 60, 'G');
  dump_flags(fd_out->w, var->flags, VAR_FLAG_NAMES, 0);

  buffer_putnlflush(fd_out->w);
}
