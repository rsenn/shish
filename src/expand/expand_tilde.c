#include "../expand.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/byte.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <pwd.h>
#include <unistd.h>
#endif

/* Known gap: this module only checks a chunk's own S_TABLE quoting
 * state, not per-byte escaping within it, so an escaped '~' ("\~")
 * that reduces to a plain '~' byte still gets expanded. Quoting the
 * '~' itself ("~"/'~') works correctly, since that puts it in a
 * differently-flagged chunk this code never touches.
 * ----------------------------------------------------------------------- */

/* resolve a tilde-prefix ("~" or "~name") at the very start of
 * [text,len) -- POSIX 2.6.1.
 * - empty name: resolves via $HOME, falling back to the passwd entry
 * - non-empty name: looked up via getpwnam()
 * On success, *home gets the resolved directory and prefixlen the
 * byte length of the "~name" prefix (excluding any trailing '/').
 * Returns 0, leaving both untouched, for anything not a resolvable
 * tilde-prefix -- caller's convention: "0 means leave text alone".
 * ----------------------------------------------------------------------- */
int
expand_tilde_lookup(
    const char* text, size_t len, int stop_at_colon, stralloc* home, size_t* prefixlen) {
  size_t namelen;

  if(!len || text[0] != '~')
    return 0;

  namelen = 0;

  while(namelen + 1 < len && text[1 + namelen] != '/' &&
        !(stop_at_colon && text[1 + namelen] == ':'))
    namelen++;

  if(namelen == 0) {
    size_t hlen;
    const char* h = var_vdefault("HOME", NULL, &hlen);

    if(h) {
      stralloc_copyb(home, h, hlen);
    } else {
#if !WINDOWS_NATIVE
      struct passwd* pw = getpwuid(getuid());

      if(!pw)
        return 0;

      stralloc_copys(home, pw->pw_dir);
#else
      return 0;
#endif
    }
  } else {
#if !WINDOWS_NATIVE
    stralloc namesa;
    struct passwd* pw;

    stralloc_init(&namesa);
    stralloc_copyb(&namesa, text + 1, namelen);
    stralloc_nul(&namesa);

    pw = getpwnam(namesa.s);
    stralloc_free(&namesa);

    if(!pw)
      return 0;

    stralloc_copys(home, pw->pw_dir);
#else
    return 0;
#endif
  }

  stralloc_nul(home);
  *prefixlen = 1 + namelen;
  return 1;
}

/* splits a chunk in two at "prefixlen": the front becomes the resolved
 * home directory, marked S_DQUOTED so it's exempt from field
 * splitting/globbing (POSIX). A new sibling chunk holds whatever text
 * followed the prefix, keeping the original flags so it behaves as if
 * the tilde-prefix had never been there. */
static void
expand_tilde_splice(union node* n, stralloc* home, size_t prefixlen) {
  union node* rest = tree_newnode(N_ARGSTR);

  stralloc_init(&rest->nargstr.stra);
  stralloc_catb(&rest->nargstr.stra,
                n->nargstr.stra.s + prefixlen,
                n->nargstr.stra.len - prefixlen);
  stralloc_nul(&rest->nargstr.stra);
  rest->nargstr.flag = n->nargstr.flag;
  rest->nargstr.loc = n->nargstr.loc;
  rest->next = n->next;

  stralloc_free(&n->nargstr.stra);
  stralloc_init(&n->nargstr.stra);
  stralloc_cat(&n->nargstr.stra, home);
  stralloc_nul(&n->nargstr.stra);
  n->nargstr.flag = S_DQUOTED;
  n->next = rest;
}

/* rewrites a plain command-argument word in place: a tilde-prefix only
 * ever applies at the very start of the word, so only the first chunk
 * (and only if it's unquoted literal text) is ever eligible.
 * ----------------------------------------------------------------------- */
void
expand_tilde_word(union node* arg) {
  union node* n;
  stralloc home;
  size_t prefixlen;

  if(!arg || arg->id != N_ARG || !(n = arg->narg.list))
    return;

  if(n->id != N_ARGSTR || (n->nargstr.flag & S_TABLE) != S_UNQUOTED)
    return;

  stralloc_init(&home);

  if(expand_tilde_lookup(n->nargstr.stra.s, n->nargstr.stra.len, 0, &home, &prefixlen))
    expand_tilde_splice(n, &home, prefixlen);

  stralloc_free(&home);
}

/* same idea for an assignment word ("NAME=value..."): the value begins
 * right after the always-literal "NAME=". From there, walks the chunk
 * list as a state machine tracking a ":"-or-value-start boundary (true
 * at the value's start and after any unquoted literal ':', false
 * otherwise), resolving+splicing a tilde-prefix at each one.
 * ----------------------------------------------------------------------- */
void
expand_tilde_assign(union node* var) {
  union node* n;
  size_t eq;
  int at_boundary;

  if(!var || var->id != N_ARG || !(n = var->narg.list))
    return;

  if(n->id != N_ARGSTR || (n->nargstr.flag & S_TABLE) != S_UNQUOTED)
    return;

  eq = byte_chr(n->nargstr.stra.s, n->nargstr.stra.len, '=');

  if(eq >= n->nargstr.stra.len)
    return;

  eq++; /* the value (where tilde-prefixes are eligible) starts right
           after the '=', not on it */

  at_boundary = 1;

  while(n) {
    if(n->id != N_ARGSTR || (n->nargstr.flag & S_TABLE) != S_UNQUOTED) {
      at_boundary = 0;
      n = n->next;
      continue;
    }

    while(eq < n->nargstr.stra.len) {
      if(at_boundary) {
        stralloc home;
        size_t prefixlen;

        stralloc_init(&home);

        if(expand_tilde_lookup(
               n->nargstr.stra.s + eq, n->nargstr.stra.len - eq, 1, &home, &prefixlen)) {
          /* splice into just the tail starting at eq, then keep
             scanning right after the replacement -- the chunk's
             length/content just changed under us */
          stralloc tail;

          stralloc_init(&tail);
          stralloc_catb(&tail, n->nargstr.stra.s, eq);
          stralloc_cat(&tail, &home);
          stralloc_catb(&tail,
                        n->nargstr.stra.s + eq + prefixlen,
                        n->nargstr.stra.len - eq - prefixlen);
          stralloc_nul(&tail);

          eq += home.len;

          stralloc_free(&n->nargstr.stra);
          n->nargstr.stra = tail;
        }

        stralloc_free(&home);
        at_boundary = 0;
      }

      if(eq < n->nargstr.stra.len && n->nargstr.stra.s[eq] == ':')
        at_boundary = 1;
      else
        at_boundary = 0;

      eq++;
    }

    eq = 0;
    n = n->next;
  }
}
