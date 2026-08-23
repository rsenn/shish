#include "../features.h"
#include "../expand.h"
#include "../job.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../fdtable.h"
#include "../var.h"
#include "../term.h"

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
limit(const struct range* in, int from, int to) {
  struct range out;

  if(in->offset < 0 && (-in->offset) <= to)
    out.offset = to + in->offset;
  else
    out.offset = clamp(in->offset, from, to);

  out.length = clamp(out.offset + in->length, from, to) - out.offset;
  return out;
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
  union node *start = *nptr, *n = *nptr;
  stralloc value;
  const char* v = NULL;
  size_t vlen = 0;
  bool is_set = true;

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
        size_t i;
        const char* ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

        for(i = 0; i < sh->arg.c; i++) {
          if(i > 0)
            stralloc_catc(&value, ifs[0]);

          stralloc_cats(&value, sh->arg.v[i]);
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
        struct nargparam arg = {
            N_ARGPARAM, S_ARG | (param->flag & S_VAR), NULL, NULL, param->word, 0};

        for(i = r.offset, e = r.offset + r.length; i < e;) {

          // param->flag |= S_ARG;
          arg.numb = 1 + i;

          n = expand_param(&arg, nptr, flags);
          i++;

          /* n is NULL when this index contributed no node (an empty
             positional parameter dropped by field splitting) -- keep
             nptr as-is instead of computing a bogus "&NULL->next". */
          if(n && i < sh->arg.c)
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
      case S_FLAGS:
        stralloc_ready(&value, 16);
        value.len = sh_fmtflags(value.s, &sh->opts);
        break;

      /* $! substitution: pid of the most recently backgrounded
         command, or empty if nothing has been backgrounded yet */
      case S_BGEXCODE:
        if(job_bgpid)
          stralloc_catulong0(&value, job_bgpid, 0);
        else
          is_set = false;
        break;

        /* $[0-9] arg subst */
      case S_ARG: {
        if(param->numb == 0)
          stralloc_cats(&value, sh_argv0);
        else if((unsigned)(param->numb - 1) < sh->arg.c)
          stralloc_cats(&value, (sh->arg.v /* + sh->arg.s*/)[param->numb - 1]);
        else
          is_set = false;

        break;
      }

        /* $$ arg subst */
      case S_PID: {
        stralloc_catulong0(&value, sh_shpid, 0);
        break;
      }
    }

    /* special parameters are always set, even when their value is the
       empty string -- e.g. an empty positional parameter inside "$@"
       must still produce an (empty) word, not be treated as unset and
       dropped entirely by the S_DEFAULT case below */
    stralloc_nul(&value);

    if(is_set) {
      v = value.s;
      vlen = value.len;
    }
  }

  /* ..and variable substitutions */
  else {
    size_t offset;
    char tmpbuf[FMT_ULONG];

    if(var_random_active && str_equal(param->name, "RANDOM")) {
      uint16 random = var_random_next();

      v = tmpbuf;
      vlen = fmt_ulong(tmpbuf, random);

      /* $LINENO:  source line of the reference, captured at parse time.
         $LINES:   terminal rows
         $COLUMNS: terminal columns

         Static buffer is safe since v is consumed by the immediately-
         following expand_cat below. */
    } else if(str_equal(param->name, "LINENO")
#ifdef HAVE_WINSIZE
              || (str_equal(param->name, "LINES") && term_size.ws_row) ||
              (str_equal(param->name, "COLUMNS") && term_size.ws_col)
#endif
    ) {
      unsigned long n;

      switch(param->name[4]) {
#ifdef HAVE_WINSIZE
        case 'S': n = term_size.ws_row; break;
        case 'M': n = term_size.ws_col; break;
#endif
        default: n = param->loc.line; break;
      }

      vlen = fmt_ulong(tmpbuf, n);
      tmpbuf[vlen] = '\0';
      v = tmpbuf;

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
      /* POSIX exempts "${parameter:-word}" and friends ("=", "?", "+")
         from nounset -- each has its own defined behavior for unset,
         handled below once v is left NULL here. Only a bare
         ${parameter}/$parameter is what nounset actually guards. */
    } else if(sh->opts.unset && (param->flag & S_VAR) == S_DEFAULT && !param->word) {
      sh_msg(param->name);
      buffer_putsflush(fd_err->w, ": unbound variable\n");

      /* POSIX: a non-interactive shell must exit outright here, not
         just fail this one word. Avoids having to free/unlink `n`,
         which is a placeholder already linked into the caller's
         argument list. */
      if(!(source->mode & SOURCE_IACTIVE))
        sh_exit(1);

      expand_error = 1;
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
        n = expand_arg(param->word, nptr, flags | X_SUBWORD);

      break;
    }
    /* if parameter unset (or null) then expand word to it and substitute paramter */
    case S_ASGNDEF: {
      if(v)
        n = expand_cat(v, vlen, nptr, flags);
      else {
        /* nptr's node may already hold text from earlier pieces of
           the surrounding word; expand_arg() below appends onto that
           same node, so only the bytes after this snapshot are the
           value to assign. */
        size_t before = n ? n->narg.stra.len : 0;

        n = expand_arg(param->word, nptr, flags | X_NOSPLIT);
        var_setv(param->name, n->narg.stra.s + before, n->narg.stra.len - before, V_DEFAULT);
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

        /* POSIX 2.8.1: an expansion error ends a non-interactive
           shell; an interactive one just fails this command. */
        if(!(source->mode & SOURCE_IACTIVE))
          sh_exit(1);

        expand_error = 1;
        n = 0;
        goto fail;
      }

      break;
    }

    /* if parameter unset (or null) then substitute null, otherwise substitute word */
    case S_ALTERNAT: {
      if(v)
        n = expand_arg(param->word, nptr, flags | X_SUBWORD);
      break;
    }

    /* remove smallest matching suffix */
    case S_RSSFX: {
      int i;
      stralloc sa;

      /* ${var#pattern}/${var%pattern} and friends aren't pathname
         expansion (POSIX 2.6.2 has no "leading dot must be matched
         explicitly" rule for this notation), so SH_FNM_PERIOD must
         not be passed here. */
      if(v && vlen) {
        expand_copysa(param->word, &sa, 0);

        for(i = vlen - 1; i >= 0; i--)
          if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, 0) == 0)
            break;

        n = expand_cat(v, (i < 0 ? vlen : (size_t)i), nptr, flags);
      }

      break;
    }

    /* remove largest matching suffix */
    case S_RLSFX: {
      unsigned int i;
      stralloc sa;

      if(v && vlen) {
        expand_copysa(param->word, &sa, 0);

        for(i = 0; i <= vlen; i++)
          if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, 0) == 0)
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
          if(path_fnmatch(sa.s, sa.len, v, i, 0) == 0)
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
          if(path_fnmatch(sa.s, sa.len, v, i, 0) == 0)
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
