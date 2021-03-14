#include "../fd.h"
#include "../var.h"
#include "../vartab.h"

/* ----------------------------------------------------------------------- */
void
vartab_dump(struct vartab* vartab) {
  unsigned int i;
  struct var* var;

  /*          buffer_puts(fd_out->w, "44: ");
            buffer_putulong(fd_out->w, (vartab->table[44]));
            buffer_putnlflush(fd_out->w);
            buffer_puts(fd_out->w, "46: ");
            buffer_putulong(fd_out->w, (vartab->table[46]));
            buffer_putnlflush(fd_out->w);*/

  buffer_putspad(fd_out->w, "address ", sizeof(var) * 2);
  buffer_puts(fd_out->w,
              " name                     value                   nlen "
              "offs vlen lev buck lexhash          rndhash        flags\n");
  buffer_putnc(fd_out->w, '-', 111 + sizeof(var) * 2);
  buffer_putc(fd_out->w, '\n');

  if(vartab) {
    for(; vartab; vartab = vartab->parent) {
      buffer_puts(fd_out->w, "level: ");
      buffer_putulong(fd_out->w, vartab->level);
      buffer_putc(fd_out->w, '\n');
      buffer_putnc(fd_out->w, '=', 111 + sizeof(var) * 2);
      buffer_putnlflush(fd_out->w);

      for(i = 0; i < (unsigned int)VARTAB_BUCKETS; i++) {
        for(var = vartab->table[i]; var; var = var->bnext) {
          var_dump(var);
        }
      }
    }
  } else {
    for(var = var_list; var; var = var->gnext) var_dump(var);
  }
}
