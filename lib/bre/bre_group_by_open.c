#include "../bre.h"

int
bre_group_by_open(struct bre_ctx* ctx, const char* pat) {
  for(int i = 0; i < ctx->nopen; i++)
    if(ctx->open_ptr[i] == pat)
      return i;
  return -1;
}