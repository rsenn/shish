#include "../filter.h"
#include "../../lib/alloc.h"

void
filter_close(const struct filter_ops* ops, void* ctx) {
  if(ops->finish)
    ops->finish(ctx);

  filter_in_close(ctx);
  alloc_free(ctx);
}
