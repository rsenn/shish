#include "../bre.h"
#include "../stralloc.h"

void
bre_subst_expand(const char* repl, const struct bre_ctx* ctx, stralloc* out) {
  const char* p = repl;
  while(*p) {
    if(*p == '&') {
      if(ctx->gstart[0] && ctx->gend[0])
        stralloc_catb(out, ctx->gstart[0], (size_t)(ctx->gend[0] - ctx->gstart[0]));
      p++;
    } else if(*p == '\\' && p[1] >= '1' && p[1] <= '9') {
      int idx = p[1] - '1';
      if(ctx->gstart[idx] && ctx->gend[idx])
        stralloc_catb(out, ctx->gstart[idx], (size_t)(ctx->gend[idx] - ctx->gstart[idx]));
      p += 2;
    } else if(*p == '\\' && p[1] == '\\') {
      stralloc_catc(out, '\\');
      p += 2;
    } else {
      stralloc_catc(out, *p++);
    }
  }
}
