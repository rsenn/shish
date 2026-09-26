#include "../bre.h"

void
bre_prepass(struct bre_ctx* ctx, const char* pat) {
  int stack[BRE_MAXGROUPS];
  int sp = 0;
  ctx->nopen = 0;
  for(; *pat; pat++) {
    if(pat[0] == '\\' && pat[1] == '(') {
      if(ctx->nopen < BRE_MAXGROUPS) {
        ctx->open_ptr[ctx->nopen] = pat;
        if(sp < BRE_MAXGROUPS)
          stack[sp++] = ctx->nopen;
        ctx->nopen++;
      }
      pat++;
    } else if(pat[0] == '\\' && pat[1] == ')') {
      if(sp > 0)
        ctx->close_ptr[stack[--sp]] = pat;
      pat++;
    } else if(pat[0] == '\\' && pat[1]) {
      pat++;
    }
  }
}