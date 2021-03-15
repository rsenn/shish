#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef _WIN32
#include <windows.h>
#else
#define _MISC_SOURCE
#include <unistd.h>
#endif

#include "../parse.h"
#include "../prompt.h"
#include "../../lib/scan.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../tree.h"
#include "../var.h"

stralloc prompt_hostname = {NULL, 0, 0};

/* handles prompt escape sequences
 * ----------------------------------------------------------------------- */
void
prompt_escape(const char* s, stralloc* sa) {
  for(; *s; s++) {
    if(*s != '\\') {
      stralloc_catc(sa, *s);
      continue;
    }

    /* parse octal escape seq */
    if(parse_isdigit(s[1])) {
      unsigned long v;

      s += scan_8long(s + 1, &v);
      stralloc_catc(sa, v);
      continue;
    }

    switch(s[1]) {
      /* escape seq */
      case 'e':
        stralloc_catc(sa, 0x1b);
        s++;
        break;

      /* hostname */
      case 'h':
        if(!prompt_hostname.s)
          shell_gethostname(&prompt_hostname);
        if(prompt_hostname.s)
          stralloc_cat(sa, &prompt_hostname);
        s++;
        break;

      /* root (#) or user ($) */
      case '$':
        stralloc_catc(sa, (sh_uid ? '$' : '#'));
        s++;
        break;

      /* shell basename */
      case 's':
        stralloc_cats(sa, sh_name);
        s++;
        break;

#ifdef PACKAGE_VERSION
      /* shell version */
      case 'v':
        stralloc_cats(sa, PACKAGE_VERSION);
        s++;
        break;
#endif

      /* working dir */
      case 'w':
        if(stralloc_starts(&sh->cwd, sh_home)) {
          size_t hlen = str_len(sh_home);
          stralloc_catc(sa, '~');
          stralloc_catb(sa, sh->cwd.s + hlen, sh->cwd.len - hlen);
        } else {
          stralloc_cat(sa, &sh->cwd);
        }
        s++;
        break;

      case 'u': {
#if WINDOWS_NATIVE
        DWORD len = 64;
        stralloc_readyplus(sa, len);
        if(GetUserNameA(sa->s, &len))
          sa->len += len;
#else
        size_t len;
        const char* user = var_value("USER", &len);

        if(user && len)
          stralloc_catb(sa, user, len);
        else
          stralloc_catulong(sa, getuid());
#endif
        s++;
        break;
      }

      /* bash shit */
      case '[':
      case ']': s++; break;

      default: stralloc_catc(sa, *s); break;
    }
  }
}
