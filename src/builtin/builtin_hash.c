#include "../../lib/buffer.h"
#include "../exec.h"
#include "../fd.h"
#include "../../lib/fmt.h"

/* hash built-in
 *
 * all your drugs are belong to us
 * ----------------------------------------------------------------------- */
int
builtin_hash(int argc, char** argv) {
  unsigned int i, n;
  struct exechash* h;
  char num[FMT_ULONG];

  buffer_puts(fd_out->w, "hits    command\n");

  for(i = 0; i < EXEC_HASHSIZE; i++) {
    for(h = exec_hashtbl[i]; h; h = h->next) {
      /* print hits right-aligned */
      n = fmt_ulong(num, h->hits);
      num[n] = '\0';
      buffer_putnspace(fd_out->w, (n = 4 - n));
      buffer_puts(fd_out->w, num);

      /* print command left aligned in next column */
      buffer_putnspace(fd_out->w, 4 + (n > 0 ? 0 : n));

      switch(h->id) {
        case H_PROGRAM: buffer_puts(fd_out->w, h->cmd.path); break;
        case H_EXEC:
        case H_SBUILTIN: buffer_putm_internal(fd_out->w, h->name, " (special builtin)", NULL); break;
        case H_BUILTIN: buffer_putm_internal(fd_out->w, h->name, " (builtin)", NULL); break;
        case H_FUNCTION: buffer_putm_internal(fd_out->w, h->name, " (function)", NULL); break;
      }
      buffer_putnlflush(fd_out->w);
    }
  }

  return 0;
}
