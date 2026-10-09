#include "../path_internal.h"
#include "../windoze.h"

/* path_canonicalize() with a relative <path> resolved against <cwd> first
 * (the current directory when <cwd> is NULL or empty), so the result is
 * absolute. <sa> is replaced, not appended to.
 *
 *   "" and "." -> the cwd
 * ----------------------------------------------------------------------- */
int
path_realpath(const char* path, stralloc* sa, int symbolic, stralloc* cwd) {
  stralloc own, full;
  int ret = 0;

  stralloc_init(&own);
  stralloc_init(&full);

  if(!path_is_absolute(path)) {
    if(!cwd || !cwd->len) {
      if(!path_getcwd(&own))
        goto end;
      cwd = &own;
    }

    if(!stralloc_copyb(&full, cwd->s, cwd->len) || !stralloc_catc(&full, PATHSEP_C) || !stralloc_cats(&full, path) ||
       !stralloc_nul(&full))
      goto end;

    path = full.s;
  }

  ret = path_canonicalize(path, sa, symbolic);

end:
  stralloc_free(&own);
  stralloc_free(&full);
  return ret;
}
