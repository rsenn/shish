#include "../bre.h"

void
bre_compile(struct bre_ctx* ctx, const char* pat) {
  ctx->pat_start = pat;
  bre_prepass(ctx, pat);
  for(int i = 0; i < BRE_MAXGROUPS; i++) {
    ctx->gstart[i] = (const char*)0;
    ctx->gend[i] = (const char*)0;
  }
}