#include "../utf8.h"

static int
ci_eq(char a, char b) {
  return (a >= 'A' && a <= 'Z' ? a + 32 : a) == b;
}

/* 1 if the first non-empty of LC_ALL, LC_CTYPE, LANG (each may be NULL) names
 * a UTF-8 codeset: "en_US.UTF-8", "C.utf8", "de_DE.UTF-8@euro"; else 0 */
int
u8locale(const char* lc_all, const char* lc_ctype, const char* lang) {
  const char* v = lc_all && *lc_all ? lc_all : lc_ctype && *lc_ctype ? lc_ctype : lang;
  const char* want = "utf8";

  if(!v)
    return 0;

  while(*v && *v != '.')
    v++;

  if(*v == '.')
    v++;
  else
    return 0;

  for(; *want; v++) {
    if(*v == '-' || *v == '_')
      continue;

    if(!ci_eq(*v, *want))
      return 0;

    want++;
  }

  return *v == 0 || *v == '@';
}
