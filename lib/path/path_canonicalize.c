#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include "../byte.h"
#include "../str.h"
#include "../windoze.h"
#include "../path_internal.h"
#include "../unix.h"

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#ifndef ELOOP
#define ELOOP 40
#endif

/* links followed before giving up with ELOOP */
#define MAXSYMLINKS 40

#if WINDOWS_NATIVE
int is_symlink(const char*);
#endif

/* is path a symbolic link? (one lstat per call) */
static int
is_link(const char* path) {
#if WINDOWS_NATIVE
  if(is_symlink(path))
    return 1;
#else
  struct stat st;

  if(lstat(path, &st) != -1)
    return S_ISLNK(st.st_mode);
#endif
  return 0;
}

static void
pop(stralloc* sa, size_t root) {
  size_t k = sa->len - path_rcomlen(sa->s + root, sa->len - root);

  if(k > root)
    k -= path_rseplen(sa->s + root, k - root);

  sa->len = k;
}

/* canonicalizes <path> and replaces the content of <sa> (NUL-terminated) with it
 *
 *   ".", ".." and repeated separators are resolved lexically; a trailing
 *   separator is dropped; a relative result is never empty ("." instead);
 *   ".." above the root of an absolute path is dropped.
 *
 * <symbolic> != 0 keeps symlinks (cd -L); zero follows each one before the
 * component after it is applied (cd -P, realpath -P). Components that do not
 * exist are kept as they are.
 *
 * Returns 0 on error (errno set; ELOOP after MAXSYMLINKS links), otherwise
 * 1 + the number of symlinks that were followed.
 *
 * <path> may be relative to the current directory and the result then is
 * relative too; path_realpath() makes it absolute.
 * ----------------------------------------------------------------------- */
int
path_canonicalize(const char* path, stralloc* sa, int symbolic) {
  stralloc rest, link, tmp;
  size_t pos = 0, root = 0, n;
  int ret = 0, links = 0;

  stralloc_init(&rest);
  stralloc_init(&link);
  stralloc_init(&tmp);

  /* path may point into sa, so copy before sa is cleared */
  if(!stralloc_copys(&rest, path))
    goto fail;

  sa->len = 0;

restart:
  if((root = path_rootlenb(rest.s, rest.len))) {
    if(!stralloc_catb(sa, rest.s, root))
      goto fail;

    sa->s[root - 1] = PATHSEP_C;
    pos = root;
  }

  while(pos < rest.len) {
    const char* c;

    if((pos += path_seplen(rest.s + pos, rest.len - pos)) == rest.len)
      break;

    c = rest.s + pos;
    n = path_comlen(c, rest.len - pos);
    pos += n;

    if(path_isdotb(c, n))
      continue;

    if(path_isdotdotb(c, n)) {
      size_t j = path_rcomlen(sa->s, sa->len);
      size_t k = sa->len - j;

      /* relative with nothing to pop (or a ".." on top): keep the ".." */
      if(sa->len > root && !(path_isdotdotb(sa->s + k, j)))
        pop(sa, root);
      else if(!root)
        if((sa->len && !stralloc_catc(sa, PATHSEP_C)) || !stralloc_catb(sa, "..", 2))
          goto fail;

      continue;
    }

    if((sa->len > root && !stralloc_catc(sa, PATHSEP_C)) || !stralloc_catb(sa, c, n) || !stralloc_nul(sa))
      goto fail;

    if(symbolic || !is_link(sa->s))
      continue;

    if(++links > MAXSYMLINKS) {
      errno = ELOOP;
      goto fail;
    }

    if(path_readlink(sa->s, &link) < 0)
      goto fail;

    /* the link's target, then what is still to do, becomes the new rest */
    pop(sa, root);

    if(!stralloc_copyb(&tmp, link.s, link.len) || !stralloc_catc(&tmp, PATHSEP_C) ||
       !stralloc_catb(&tmp, rest.s + pos, rest.len - pos) || !stralloc_nul(&tmp))
      goto fail;

    stralloc_free(&rest);
    rest = tmp;
    stralloc_init(&tmp);
    pos = 0;

    /* absolute target: start over from its root */
    if(path_isabsb(rest.s, rest.len)) {
      sa->len = 0;
      goto restart;
    }
  }

  if(sa->len == 0 && !stralloc_catc(sa, '.'))
    goto fail;

  if(stralloc_nul(sa))
    ret = 1 + links;

fail:
  stralloc_free(&rest);
  stralloc_free(&link);
  stralloc_free(&tmp);
  return ret;
}
