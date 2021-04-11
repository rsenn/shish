#include "../features.h"
#include "../expand.h"
#include "../sh.h"
#include "../tree.h"
#include "../fdtable.h"
#include "../var.h"
#include "../vartab.h"

#include "../../lib/uint16.h"
#include "../../lib/uint32.h"
#include "../../lib/fmt.h"
#include "../../lib/path.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"

#include <stdlib.h>
#include <stdint.h>

struct range {
  int32_t offset;
  int32_t length;
};

static inline int
clamp(int value, int from, int to) {
  return value < from ? from : value > to ? to : value;
}

static inline struct range
limit(const struct range* r, int from, int to) {
  struct range ret;

  if(r->offset < 0 && (-r->offset) <= to)
    ret.offset = to + r->offset;
  else
    ret.offset = clamp(r->offset, from, to);

  ret.length = clamp(ret.offset + r->length, from, to) - ret.offset;
  return ret;
}

static int
expand_range(union node* word, struct range* r) {
  stralloc sa;
  size_t p, q;
  expand_copysa(word, &sa, 0);
  stralloc_nul(&sa);

  if((p = scan_int(sa.s, &r->offset)) > 0 && p < sa.len && sa.s[p] == ':') {
    p++;
    if((q = scan_int(&sa.s[p], &r->length)) == 0)
      r->length = INT32_MAX;
  } else {
    r->offset = INT32_MIN;
  }
  stralloc_free(&sa);
  return p > 0 && r->offset != INT32_MIN;
}

union node*
expand_param(struct nargparam* param, union node** nptr, int flags) {
  union node* n = *nptr;
  stralloc value;
  const char* v = NULL;
  size_t vlen = 0;

  stralloc_init(&value);

  /* treat special arguments */
  if(param->flag & S_SPECIAL) {
    switch(param->flag & S_SPECIAL) {
      /* $# substitution */
      case S_ARGC: {
        stralloc_catulong0(&value, sh->arg.c, 0);
        break;
      }

      /* $* substitution */
      case S_ARGV: {
        char** s;
        const char* ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

        for(s = sh->arg.v + sh->arg.s; *s;) {
          stralloc_cats(&value, *s);
          if(*++s)
            stralloc_catc(&value, ifs[0]);
        }
        break;
      }

      /* $@ substitution */
      case S_ARGVS: {
        unsigned int i, e;
        struct range r = {0, sh->arg.c};
#if WITH_PARAM_RANGE
        if((param->flag & S_VAR) == S_RANGE) {
          if(expand_range(param->word, &r)) {
            r = limit(&r, 0, sh->arg.c);
          } else {

            if(r.offset == INT32_MIN)
              sh_msg("expecting offset\n");
            else if(r.length == INT32_MAX)
              sh_msg("expecting length\n");
            return n;
          }
        }
#endif
        for(i = r.offset, e = r.offset + r.length; i < e;) {
          param->flag &= ~(int)(S_SPECIAL /*| S_VAR*/);
          param->flag |= S_ARG;
          param->numb = 1 + i;

          n = expand_param(param, nptr, flags);

          if(++i < sh->arg.c)
            nptr = &n->next;
        }

        return n;
      }

        /* $? substitution */
      case S_EXITCODE: {
        stralloc_catulong0(&value, sh->exitcode, 0);
        break;
      }

      /* $- substitution */
      case S_FLAGS: break;

      /* $! substitution */
      case S_BGEXCODE:
        break;

        /* $[0-9] arg subst */
      case S_ARG: {
        if(param->numb == 0)
          stralloc_cats(&value, sh_argv0);
        else if((unsigned)(param->numb - 1) < sh->arg.c)
          stralloc_cats(&value, (sh->arg.v + sh->arg.s)[param->numb - 1]);

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
    size_t offset;

    if(str_equal(param->name, "RANDOM")) {
      char tmpbuf[FMT_ULONG];
      uint16 random = uint32_random();

      v = tmpbuf;
      vlen = fmt_ulong(tmpbuf, random);

      /* look for the variable.
         if the S_NULL flag is set and we have a var which is null
         set v to NULL */
    } else if((v = var_get(param->name, &offset))) {
      if(v[offset] == '\0' && (param->flag & S_NULL)) {
        v = NULL;
        vlen = 0;
      } else {
        v = &v[offset];
        vlen = str_len(v);
      }
    } else if(sh->opts.unset) {
      vartab_dump(varstack, 1, &param->name);

      sh_msg(param->name);
      buffer_putsflush(fd_err->w, ": unbound variable\n");
      tree_free(n);

      n = 0;
      goto fail;
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
  switch(param->flag & S_VAR) {
    /* return word if parameter unset (or null) */
    case S_DEFAULT: {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      /* unset, substitute */
      else if(param->word)
        n = expand_arg(param->word, nptr, flags);
      break;
    }
    /* if parameter unset (or null) then expand word to it
       and substitute paramter */
    case S_ASGNDEF: {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      else {
        n = expand_arg(param->word, nptr, flags | X_NOSPLIT);
        var_setvsa(param->name, /* BUG */ &n->narg.stra, V_DEFAULT);
      }
      break;
    }

    /* indicate error if null or unset */
    case S_ERRNULL: {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      else {
        stralloc msg = {0, 0, 0};

        if(param->word)
          expand_copysa(param->word, &msg, flags);
        sh_error(param->word ? msg.s : "parameter null or not set");
        stralloc_free(&msg);
        sh_exit(1);
      }
      break;
    }

      /* if parameter unset (or null) then substitute null,
         otherwise substitute word */
    case S_ALTERNAT: {
      if(v)
        n = expand_arg(param->word, nptr, flags);
      break;

        /* remove smallest matching suffix */
      case S_RSSFX: {
        int i;
        stralloc sa;

        if(v && vlen) {
          expand_copysa(param->word, &sa, 0);

          for(i = vlen - 1; i >= 0; i--)
            if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, SH_FNM_PERIOD) == 0)
              break;

          n = expand_cat(v, (i < 0 ? vlen : (size_t)i), nptr, flags);
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
          if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, SH_FNM_PERIOD) == 0)
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
          if(path_fnmatch(sa.s, sa.len, v, i, SH_FNM_PERIOD) == 0)
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
          if(path_fnmatch(sa.s, sa.len, v, i, SH_FNM_PERIOD) == 0)
            break;

        if(i == 0)
          i = vlen;

        n = expand_cat(v + i, vlen - i, nptr, flags);
      }
      break;
    }

#if WITH_PARAM_RANGE
    /* character or field range */
    case S_RANGE: {
      if(v && vlen) {
        struct range r = {0, vlen};
        if(expand_range(param->word, &r)) {
          r = limit(&r, 0, vlen);

          n = expand_cat(v + r.offset, r.length, nptr, flags);
        } else {

          if(r.offset == INT32_MIN)
            sh_msg("expecting offset\n");
          else if(r.length == INT32_MAX)
            sh_msg("expecting length\n");
        }
      }
      break;
    }
#endif
  }

fail:
  stralloc_free(&value);
  return n;
}
