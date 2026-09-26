#include "../bre.h"

int
bre_match_exec(struct bre_ctx* ctx, const char* pat, const char* str) {
  bre_compile(ctx, pat);
  if(pat[0] == '^') {
    return bre_match_rec(ctx, pat, str) != (const char*)0;
  }
  const char* p = str;
  while(1) {
    for(int i = 0; i < BRE_MAXGROUPS; i++) {
      ctx->gstart[i] = (const char*)0;
      ctx->gend[i] = (const char*)0;
    }
    if(bre_match_rec(ctx, pat, p) != (const char*)0)
      return 1;
    if(*p == '\0')
      break;
    p++;
  }
  return 0;
}