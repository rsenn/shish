#include "../bre.h"
#include "../byte.h"

const char*
bre_match_rec(struct bre_ctx* ctx, const char* pat, const char* s) {
  if(*pat == 0)
    return s;
  if(pat[0] == '\\' && pat[1] == '(') {
    int idx = bre_group_by_open(ctx, pat);
    if(idx >= 0)
      ctx->gstart[idx] = s;
    return bre_match_rec(ctx, pat + 2, s);
  }
  if(pat[0] == '\\' && pat[1] == ')') {
    int idx = bre_group_by_close(ctx, pat);
    if(idx >= 0)
      ctx->gend[idx] = s;
    return bre_match_rec(ctx, pat + 2, s);
  }
  if(pat[0] == '\\' && pat[1] >= '1' && pat[1] <= '9') {
    int idx = pat[1] - '1';
    const char* start = ctx->gstart[idx];
    const char* end = ctx->gend[idx];
    if(!start || !end)
      return (const char*)0;
    size_t len = (size_t)(end - start);
    if(byte_diff(s, len, start) != 0)
      return (const char*)0;
    return bre_match_rec(ctx, pat + 2, s + len);
  }
  if(pat[0] == '^' && pat == ctx->pat_start)
    return bre_match_rec(ctx, pat + 1, s);
  if(pat[0] == '$' && pat[1] == 0)
    return *s == 0 ? s : (const char*)0;
  {
    size_t alen = bre_atom_len(pat);
    if(pat[alen] == '*')
      return bre_match_star(ctx, pat, alen, pat + alen + 1, s);
    if(*s == 0 || !bre_atom_matches(pat, alen, (unsigned char)*s))
      return (const char*)0;
    return bre_match_rec(ctx, pat + alen, s + 1);
  }
}
