#include "../expand.h"
#include "../fd.h"
#include <assert.h>

void
expand_dump(const struct expand* ex) {
  buffer_puts(fd_err->w, "root: ");
  buffer_putxlonglong(fd_err->w, ex->root);
  buffer_puts(fd_err->w, "\nptr: ");
  buffer_putxlonglong(fd_err->w, ex->ptr);
  buffer_puts(fd_err->w, "\nflags: ");
  buffer_putxlong(fd_err->w, ex->flags);
  buffer_putnlflush(fd_err->w);
}