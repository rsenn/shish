#include <stdlib.h>
#include "shell.h"
#include "str.h"
#include "fmt.h"
#include "sh.h"
#include "var.h"
#include "tree.h"
#include "expand.h"

union node *expand_param(struct nargparam *param, union node **nptr, int flags) {
  union node *n = *nptr;
  stralloc value;
  const char *v = NULL;
  unsigned long vlen = 0;
  
  stralloc_init(&value);

  /* treat special arguments */
  if(param->flag & S_SPECIAL)
  {
    switch(param->flag & S_SPECIAL)
    {
      /* $# substitution */
      case S_ARGC:
      {
        stralloc_catulong0(&value, sh->arg.c, 0);
        break;
      }
      
      /* $* substitution */
      case S_ARGV:
      {
        char **s;
        
        for(s = sh->arg.v; *s;)
        {
          stralloc_cats(&n->narg.stra, *s);
          if(*++s)
            stralloc_catc(&n->narg.stra, ' ');
        }
        break;
      }
      
      /* $@ substitution */
      case S_ARGVS:
      {
        unsigned int i = 0;
        
        while(i < sh->arg.c)
        {
          param->flag &= ~S_SPECIAL;
          param->flag |= S_ARG;
          param->numb = 1 + i;
        
          n = expand_param(param, nptr, flags);
          
          if(++i < sh->arg.c)
            nptr = &n->list.next;
        }
        
        return n;
      }

      /* $? substitution */
    case S_EXITCODE: {
        stralloc_catulong0(&value, sh->exitcode, 0);
        break;
      }

      /* $- substitution */
      case S_FLAGS:
        break;

      /* $! substitution */
      case S_BGEXCODE:
        break;
      
      /* $[0-9] arg subst */
    case S_ARG: {
        if(param->numb == 0)
          stralloc_cats(&value, sh_argv0);          
        else if(param->numb - 1 < sh->arg.c)
          stralloc_cats(&value, sh->arg.v[param->numb - 1]);
        
        break;
      }
      
      /* $$ arg subst */
    case S_PID: {
        stralloc_catulong0(&value, sh_pid, 0);
        break;
      }
    }
    
    /* special parameters are always set */
    if(value.len) {
      stralloc_nul(&value);
      v = value.s;
    }
    
    vlen = value.len;
  }
  /* ..and variable substitutions */
  else {
    unsigned long offset;

    /* look for the variable.
       if the S_NULL flag is set and we have a var which is null
       set v to NULL */
    if((v = var_get(param->name, &offset))) {
      if(v[offset] == '\0' && (param->flag & S_NULL)) {
        v = NULL;
        vlen = 0;
      } else {
        v = &v[offset];
        vlen = str_len(v);
      }
    }
  }
  
  /* check for S_STRLEN substitution */
  if(param->flag & S_STRLEN) {
    char lstr[FMT_ULONG];
    
    n = expand_cat(lstr, fmt_ulong(lstr, vlen), nptr, flags);
    
    stralloc_free(&value);
    
    return n;    
  }

  /* otherwise expand the apropriate variable/word subst */
  switch(param->flag & S_VAR)
  {
    /* return word if parameter unset (or null) */
    case S_DEFAULT:
    {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
        /* unset, substitute */
      else
        n = expand_arg(&param->word->narg, nptr, flags);
      break;
    }
    /* if parameter unset (or null) then expand word to it
       and substitute paramter */
    case S_ASGNDEF:
    {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      else
      {
        n = expand_arg(&param->word->narg, nptr, flags|X_NOSPLIT);
        var_setvsa(param->name, /* BUG */&n->narg.stra, V_DEFAULT);
      }
      break;
    }
    
    /* indicate error if null or unset */
    case S_ERRNULL:
    {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      else
      {
        union node *tmpnode = NULL;
        
        n = expand_arg(&param->word->narg, &tmpnode, flags);
        sh_error((n && n->narg.stra.s) ? n->narg.stra.s : "parameter null or not set");
        if(tmpnode) tree_free(tmpnode);
      }
      break;
    }
    
    /* if parameter unset (or null) then substitute null,
       otherwise substitute word */
  case S_ALTERNAT: {
      if(v)
        n = expand_arg(&param->word->narg, nptr, flags);
      break;
      
      /* remove smallest matching suffix */
    case S_RSSFX: {
        int i;
        stralloc sa;

      if(v && vlen) {
          expand_copysa(param->word, &sa, 0);

          for(i = vlen - 1; i >= 0; i--)
            if(shell_fnmatch(sa.s, sa.len, v + i, vlen - i, SH_FNM_PERIOD) == 0)
              break;

          n = expand_cat(v, (i < 0 ? vlen : i), nptr, flags);
        }
        break;
      }
    }

    /* remove largest matching suffix */
  case S_RLSFX: {
      unsigned int i;
      stralloc sa;
      
    if(v && vlen) {
        expand_copysa(param->word, &sa, 0);
        
        for(i = 0; i <= vlen; i++)
          if(shell_fnmatch(sa.s, sa.len, v + i, vlen - i, SH_FNM_PERIOD) == 0)
            break;
        
        n = expand_cat(v, (i > vlen ? vlen : i), nptr, flags);
      }
      
      break;
    }

    /* remove smallest matching prefix */
  case S_RSPFX: {
      unsigned int i;
      stralloc sa;

    if(v && vlen) {
        expand_copysa(param->word, &sa, 0);

        for(i = 1; i <= vlen; i++)
          if(shell_fnmatch(sa.s, sa.len, v, i, SH_FNM_PERIOD) == 0)
            break;

        if(i > vlen)
          i = 0;

        n = expand_cat(v + i, vlen - i, nptr, flags);
      }
      break;
    }

    /* remove largest matching prefix */
  case S_RLPFX: {
      unsigned int i;
      stralloc sa;

    if(v && vlen) {
        expand_copysa(param->word, &sa, 0);
        
        for(i = vlen; i > 0; i--)
          if(shell_fnmatch(sa.s, sa.len, v, i, SH_FNM_PERIOD) == 0)
            break;

        if(i == 0)
          i = vlen;

        n = expand_cat(v + i, vlen - i, nptr, flags);
      }
      break;
    }
  }

  stralloc_free(&value);  
  return n;
}

