#include "../filter.h"

int
filter_status(const struct filter_ops* ops, void* ctx) {
  return ops->status ? ops->status(ctx) : ((struct filter_in*)ctx)->had_error;
}
