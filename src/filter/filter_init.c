#include "../filter.h"
#include "../../lib/buffer.h"
#include "../../lib/byte.h"
#include "../../lib/shell.h"

/* ---- generic open/status/close/run behind a declarative filter_ops ---- */

int
filter_init(const struct filter_ops* ops, void* ctx, int argc, char* argv[], buffer* upstream) {
  struct filter_in* in = ctx;
  const char *err_arg, *err_msg;
  int ch;

  byte_zero(ctx, ops->size);

  if(ops->opts || ops->option)
    while((ch = shell_getopt(argc, argv, ops->opts ? ops->opts : "")) > 0)
      if(!ops->option || ops->option(ctx, ch) < 0)
        return -1;

  /* an option() error message survives filter_in_init()'s zeroing */
  err_arg = in->err_arg;
  err_msg = in->err_msg;
  filter_in_init(in, argv, argv[shell_optind] ? argv + shell_optind : NULL, upstream);
  in->err_arg = err_arg;
  in->err_msg = err_msg;
  return err_msg ? -1 : ops->setup ? ops->setup(ctx) : 0;
}
