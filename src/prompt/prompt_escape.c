#include "shell.h"
#include "scan.h"
#include "tree.h"
#include "prompt.h"
#include "parse.h"
#include "sh.h"

/* handles prompt escape sequences 
 * ----------------------------------------------------------------------- */
void prompt_escape(const char *s, stralloc *sa)
{
  for(; *s; s++)
  {
    if(*s != '\\')
    {
      stralloc_catc(sa, *s);
      continue;
    }
  
    /* parse octal escape seq */
    if(parse_isdigit(s[1]))
    {
      unsigned long v;

      s += scan_8long(s + 1, &v);
      stralloc_catc(sa, v);
      continue;
    }

    switch(s[1])
    {
      /* escape seq */
      case 'e':
        stralloc_catc(sa, 0x1b);
        s++;
        break;

      /* hostname */
      case 'h':
        if(!sh_hostname.s)
          shell_gethostname(&sh_hostname);
        if(sh_hostname.s)
          stralloc_cat(sa, &sh_hostname);
        s++;
        break;

      /* root (#) or user ($) */
      case '$':
        stralloc_catc(sa, (sh_uid ? '$' : '#'));
        s++; break;

      /* shell basename */
      case 's':
        stralloc_cats(sa, sh_name);
        s++; break;

      /* shell version */
      case 'v':
        stralloc_cats(sa, PACKAGE_VERSION);
        s++; break;

      /* working dir */
      case 'w':
        stralloc_cat(sa, &sh->cwd);
        s++; break;

      /* bash shit */
      case '[':
      case ']':
        s++; break;

      default:
        stralloc_catc(sa, *s);
        break;
    }
  }
}

