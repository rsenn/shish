#include "../bre.h"

int
bre_group_by_close(struct bre_ctx* ctx, const char* pat) {
  for(int i = 0; i < ctx->nopen; i++)
    if(ctx->close_ptr[i] == pat)
      return i;
  return -1;
}