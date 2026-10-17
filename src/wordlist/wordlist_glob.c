#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* glob() is the platform's, or lib/unix/glob.c (UTF-8, wide Win32 API) on Windows */
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#include "../../lib/glob.h"
#else
#include <glob.h>
#endif

#include "../wordlist.h"
#include "../expand.h"
#include "../parse.h"
#include "../../lib/byte.h"
#include "../../lib/str.h"

/* pathname-expand the pattern in cur
 *   match    every match but the last is pushed as a field, the last stays in cur
 *            (X_NOSPLIT: all matches joined by ifs[0] in cur)
 *   no match cur is unescaped, unless X_GLOBRES: a raw expansion result stays as it is
 * ----------------------------------------------------------------------- */
void
wordlist_glob(wordlist* wl, int flags) {
  stralloc* c = wl->cur;
  glob_t glb;

  stralloc_nul(c);

  /* a raw expansion result without a pattern character, or with -f: left exactly as it is */
  if((flags & X_GLOBRES) &&
     (wl->noglob || (!str_contains(c->s, '*') && !str_contains(c->s, '?') && !str_contains(c->s, '['))))
    return;

  /* set -f: the pattern stays literal, as in the no-match case below */
  if(wl->noglob) {
    expand_unescape(c, parse_isesc);
    return;
  }

  byte_zero(&glb, sizeof(glb));

  if(!glob(c->s, 0, NULL, &glb)) {
    size_t i;

    c->len = 0;

    for(i = 0; i < glb.gl_pathc;) {
      stralloc_cats(c, glb.gl_pathv[i]);

      if(++i < glb.gl_pathc) {
        if(flags & X_NOSPLIT) {
          if(wl->ifs && wl->ifs[0])
            stralloc_catc(c, wl->ifs[0]);
        } else {
          wordlist_push(wl);
        }

        wl->state |= flags;
      }
    }

    stralloc_nul(c);
    globfree(&glb);
    return;
  }

  if(!(flags & X_GLOBRES))
    expand_unescape(c, parse_isesc);
}
