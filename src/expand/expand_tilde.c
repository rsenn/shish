#include "../expand.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/byte.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <pwd.h>
#include <unistd.h>
#endif

/* Known, accepted gap (see tests/posix/tilde-p.tst for the exact
 * cases): POSIX also requires a tilde-prefix to stay completely
 * literal if *any* character within it (the '~', the name, or the
 * terminating '/') was quoted or escaped -- e.g. "\~", "~\foo",
 * "~'foo'", or a name that came from `~$var` expansion. Distinguishing
 * that requires per-byte quote-tracking within a chunk (or adding
 * '~' to parse_isesc's protected set, mirroring how '*'/'?'/'['
 * already survive escaping through to expand time); this module only
 * checks a whole chunk's own S_TABLE quoting state, so an escaped '~'
 * that reduces to a plain, indistinguishable '~' byte before this
 * ever runs is (incorrectly) still expanded. Quoting the '~' itself
 * ("~"/'~') still works correctly, since that already puts it in a
 * differently-flagged (non-S_UNQUOTED) chunk this code never touches.
 * ----------------------------------------------------------------------- */

/* resolve a tilde-prefix ("~" or "~name") at the very start of
 * [text,len) -- POSIX 2.6.1. An empty name resolves via $HOME (falling
 * back to the current user's passwd entry if HOME is unset); a
 * non-empty name is looked up via getpwnam(). On success, *home is
 * filled with the resolved (nul-terminated) directory and prefixlen
 * is set to how many bytes of the input the "~name" prefix itself
 * occupied (never including a trailing '/', which stays for the
 * caller to keep). Returns 0 (leaving home and prefixlen untouched)
 * for anything that isn't a resolvable tilde-prefix at all -- text not
 * starting with '~', or an unknown user -- so the caller's convention
 * is always "0 means leave the original text alone", matching
 * expand_glob.c's own "no match -> literal" fallback.
 * ----------------------------------------------------------------------- */
int
expand_tilde_lookup(const char* text, size_t len, int stop_at_colon, stralloc* home,
                     size_t* prefixlen) {
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

/* splits a chunk in two right at "prefixlen": the front becomes the
 * resolved home directory, marked S_DQUOTED so it's exempt from field
 * splitting like any other expansion result the shell itself produced
 * (POSIX: a tilde-expansion's result is not subject to field
 * splitting or pathname expansion) -- a new sibling chunk holds
 * whatever text followed the prefix, keeping the original chunk's own
 * flags (S_UNQUOTED, plus S_GLOB if it had it) so *that* part still
 * behaves exactly as if the tilde-prefix had never been there. */
static void
expand_tilde_splice(union node* n, stralloc* home, size_t prefixlen) {
  union node* rest = tree_newnode(N_ARGSTR);

  stralloc_init(&rest->nargstr.stra);
  stralloc_catb(&rest->nargstr.stra, n->nargstr.stra.s + prefixlen,
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

/* same idea for an assignment word ("NAME=value..."): the name and its
 * '=' are always literal, unquoted text at the very start of the first
 * chunk (assignment grammar guarantees this), so the value begins
 * right after that '='. From there, walks the remaining chunk list as
 * a small state machine tracking "am I at a ':'-or-value-start
 * boundary" -- true at the start of the value and immediately after
 * any unquoted literal ':', false after anything else (including any
 * substitution chunk or quoted text: a boundary must be a real
 * unquoted ':' in the source, never inferred from an expansion's
 * eventual result) -- resolving+splicing a tilde-prefix at each one.
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

        if(expand_tilde_lookup(n->nargstr.stra.s + eq, n->nargstr.stra.len - eq, 1, &home,
                                &prefixlen)) {
          /* splice into just the tail starting at eq, then keep
             scanning right after the replacement -- the chunk's
             length/content just changed under us */
          stralloc tail;

          stralloc_init(&tail);
          stralloc_catb(&tail, n->nargstr.stra.s, eq);
          stralloc_cat(&tail, &home);
          stralloc_catb(&tail, n->nargstr.stra.s + eq + prefixlen,
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
