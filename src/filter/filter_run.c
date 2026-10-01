#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/alloc.h"
#include "../../lib/open.h"
#include "../../lib/buffer.h"
#include <unistd.h>

int
filter_run(const struct filter_ops* ops, int argc, char* argv[], buffer* out) {
  void* ctx = alloc(ops->size);
  const char* name;
  buffer ob;
  char obuf[4096];
  int r, ret, fd = -1;

  if(!ctx)
    return 1;

  if((r = filter_init(ops, ctx, argc, argv, fd_in->r)) < 0) {
    struct filter_in* in = ctx;

    if(in->err_msg && in->err_arg && *in->err_arg)
      builtin_errmsg(argv, (char*)in->err_arg, (char*)in->err_msg);
    else if(in->err_msg)
      builtin_errmsg(argv, (char*)in->err_msg, NULL);
    else
      builtin_invopt(argv);

    filter_close(ops, ctx);
    return ops->err_status ? ops->err_status : 1;
  }

  if(ops->output && (name = ops->output(ctx))) {
    if((fd = open_trunc(name)) == -1) {
      builtin_error(argv, (char*)name);
      filter_close(ops, ctx);
      return 1;
    }

    buffer_init(&ob, &buffer_op_write, fd, obuf, sizeof(obuf));
    out = &ob;
  }

  ((struct filter_in*)ctx)->sink = out;

  if(((struct filter_in*)ctx)->each) {
    char** f = ((struct filter_in*)ctx)->files;

    ret = 0;

    if(f)
      for(; *f; f++)
        ret |= ops->each(ctx, *f) != 0;
    else
      ret = ops->each(ctx, NULL) != 0;
  } else {
    filter_drain(ops->step, ctx, out);
    ret = filter_status(ops, ctx);
  }

  filter_close(ops, ctx);

  if(out == &ob) {
    buffer_flush(&ob);
    close(fd);
  }

  return ret;
}
