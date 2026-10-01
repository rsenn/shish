#include "../filter.h"
#include "../../lib/alloc.h"

void*
filter_open(const struct filter_ops* ops, int argc, char* argv[], buffer* upstream) {
  void* ctx;

  if(!(ctx = alloc(ops->size)))
    return NULL;

  if(filter_init(ops, ctx, argc, argv, upstream) != 0 || ((struct filter_in*)ctx)->each ||
     (ops->output && ops->output(ctx))) {
    filter_close(ops, ctx);
    return NULL;
  }

  return ctx;
}
