#include "../path.h"

/* writes the first "element/name" of the list that ok() accepts to out and returns its length;
 * 0 if none. An element too long for cap is skipped.
 *
 *   char*  out          receives the NUL-terminated path
 *   size_t cap          its size
 *   l, ln               the PATH-style list
 *   name, nl            the bare name to look for
 *   ok(path, arg)       true if the candidate is the one (exists, is executable, ...)
 * ----------------------------------------------------------------------- */
size_t
path_search(char* out, size_t cap, const char* l, size_t ln, const char* name, size_t nl, int (*ok)(const char*, void*), void* arg) {
  const char* e;
  size_t el, n;

  PATH_LIST_FOREACH(l, ln, e, el) {
    n = path_cat(out, cap, e, el, name, nl);

    if(n < cap && ok(out, arg))
      return n;
  }

  return 0;
}
