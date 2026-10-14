#include "../sh.h"
#include "../var.h"
#include "../vartab.h"
#include "../../lib/alloc.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"
#include "../../lib/windoze.h"
#include <stdlib.h>
#include <time.h>

/* libc's localtime()/mktime() read TZ from the process environment, not from the shell's variables.
 * sh_tz_enter(tz) puts tz there (NULL: the shell's own TZ variable, removed if unset) and returns
 * the previous value for sh_tz_leave(), which restores it.
 * ----------------------------------------------------------------------- */
static void
tz_set(const char* v) {
#if WINDOWS_NATIVE
  stralloc sa;

  stralloc_init(&sa);
  stralloc_copys(&sa, "TZ=");
  stralloc_cats(&sa, v ? v : "");
  stralloc_nul(&sa);
  _putenv(sa.s);
  stralloc_free(&sa);
#else
  if(v)
    setenv("TZ", v, 1);
  else
    unsetenv("TZ");
#endif

  tzset();
}

char*
sh_tz_enter(const char* tz) {
  char* old = getenv("TZ") ? str_dup(getenv("TZ")) : 0;

  if(!tz) {
    struct var* v = var_search("TZ", 0);

    tz = v && !(v->flags & V_UNSET) ? &v->sa.s[v->offset] : 0;
  }

  tz_set(tz);
  return old;
}

void
sh_tz_leave(char* old) {
  tz_set(old);

  if(old)
    alloc_free(old);
}
