#include "../fd.h"
#include "../var.h"
#include "../vartab.h"
#include "../debug.h"
#include "../../lib/str.h"

static int
match_args(const char* x, size_t n, int argc, char* argv[]) {
  int i;
  for(i = 0; i < argc; i++) {
    size_t arglen = str_len(argv[i]);
    if(n == arglen && !str_diffn(x, argv[i], n))
      return 1;
  }
  return 0;
}

/* ----------------------------------------------------------------------- */
void
vartab_dump(struct vartab* vartab, int argc, char* argv[]) {
  unsigned int i;
  struct var* var;

  buffer_putspad(fd_out->w, "address ", sizeof(var) * 2);
  buffer_puts(fd_out->w,
              " name" CURSOR_FORWARD(16) "value" CURSOR_FORWARD(
                  16) "nlen "
                      "offs vlen lev buck lexhash          rndhash        flags\n");
  /*buffer_putnc(fd_out->w, '-', 111 + sizeof(var) * 2);
buffer_putc(fd_out->w, '\n');*/

  if(vartab) {
    for(; vartab; vartab = vartab->parent) {
      buffer_puts(fd_out->w, "─── level ");
      buffer_putulong(fd_out->w, vartab->level);
      buffer_putc(fd_out->w, ' ');
      buffer_putns(fd_out->w, "─" /*"―"*/, 99 + sizeof(var) * 2);
      buffer_putnlflush(fd_out->w);

      for(i = 0; i < (unsigned int)VARTAB_BUCKETS; i++) {
        for(var = vartab->table[i]; var; var = var->bnext) {

          if(argc > 0 && !match_args(var->sa.s, var->len, argc, argv))
            continue;

          var_dump(var);
        }
      }
    }
  } else {
    for(var = var_list; var; var = var->gnext) {
      if(argc > 0 && !match_args(var->sa.s, var->len, argc, argv))
        continue;
      var_dump(var);
    }
  }
}
