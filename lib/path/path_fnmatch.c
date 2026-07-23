#include "../path_internal.h"
#include "../byte.h"
#include <ctype.h>

/*#include "../buffer.h"*/
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support strings that are not nul-terminated
 */
#define NOTFIRST 0x80

/* does character c belong to POSIX bracket class "name" (the "lower"
 * in "[:lower:]"), name/namelen not including the surrounding "[:"/
 * ":]"? Only the classes POSIX 2.13.1 actually lists are recognized;
 * an unknown class name matches nothing, same as glibc.
 * ----------------------------------------------------------------------- */
static int
path_fnmatch_class(const char* name, unsigned int namelen, int c) {
#define CLASS(s, fn) \
  if(namelen == sizeof(s) - 1 && byte_equal(name, namelen, s)) \
  return !!fn((unsigned char)c)
  CLASS("upper", isupper);
  CLASS("lower", islower);
  CLASS("alpha", isalpha);
  CLASS("digit", isdigit);
  CLASS("alnum", isalnum);
  CLASS("punct", ispunct);
  CLASS("graph", isgraph);
  CLASS("print", isprint);
  CLASS("cntrl", iscntrl);
  CLASS("blank", isblank);
  CLASS("space", isspace);
  CLASS("xdigit", isxdigit);
#undef CLASS
  return 0;
}

/* find the end of a "[:class:]"/"[.symbol.]"/"[=equiv=]" bracket
 * sub-expression starting at "pattern" (pattern[0] == '[', pattern[1]
 * is the delimiter char "delim"): returns the number of pattern bytes
 * it occupies (including both delimiter pairs), or 0 if "plen" bytes
 * aren't enough to find a closing "delim]" -- the caller falls back
 * to treating "[" as an ordinary bracket-expression character in that
 * case, same as an unterminated "[:"/"[."/"[=" always has.
 * ----------------------------------------------------------------------- */
static unsigned int
path_fnmatch_subexpr_len(const char* pattern, unsigned int plen, char delim) {
  unsigned int i;

  for(i = 2; i + 1 < plen; i++) {
    if(pattern[i] == delim && pattern[i + 1] == ']')
      return i + 2;
  }

  return 0;
}

int
path_fnmatch(
    const char* pattern, unsigned int plen, const char* string, unsigned int slen, int flags) {
/*  buffer_puts(buffer_2, "fnmatch: ");
  buffer_put(buffer_2, pattern, plen);
  buffer_putspace(buffer_2);
  buffer_put(buffer_2, string, slen);
  buffer_putnlflush(buffer_2);*/
/* label to jump back instead of recursion */
start:
  /* when string is empty, only a pattern consisting of asteriks matches */
  if(slen == 0) {
    /* skip all asterikses */
    while(plen && *pattern == '*') {
      pattern++;
      plen--;
    }
    /* if there are chars left we don't have a match (which returns 1) */
    return (plen ? PATH_FNM_NOMATCH : 0);
  }
  /* there is still some string left but pattern ended */
  if(plen == 0)
    return PATH_FNM_NOMATCH;
  /* if PATH_FNM_PERIOD is set, a leading period in string has to be
   * matched  exactly by a period in pattern.  A period is considered
   * to be leading if it is the first character in string, or if both
   * PATH_FNM_PATHNAME  is set and the period immediately follows a slash.
   */
  if(*string == '.' && *pattern != '.' && (flags & PATH_FNM_PERIOD)) {
    /* don't match if PATH_FNM_PERIOD and this is the first char */
    if(!(flags & NOTFIRST))
      return PATH_FNM_NOMATCH;
    /* don't match if PATH_FNM_PERIOD and PATH_FNM_PATHNAME and previous was '/'
     */
    if((flags & PATH_FNM_PATHNAME) && string[-1] == '/')
      return PATH_FNM_NOMATCH;
  }
  flags |= NOTFIRST;

  switch(*pattern) {
    case '[': {
      const char* start;
      int neg = 0;
      pattern++;
      plen--;
      /* unterminated character class because in a pathname the '/' is a
         separator and can't be matched. this means we have a mismatch */
      if(*string == '/' && (flags & PATH_FNM_PATHNAME))
        return PATH_FNM_NOMATCH;
      /* exclamation mark negates the class */
      neg = (*pattern == '!');
      pattern += neg;
      plen -= neg;
      /* now start scanning the pattern */
      start = pattern;

      while(plen) {
        int res = 0;
        int escaped = 0;

        /* a backslash-escaped member is always a literal char -- it
           can't close the bracket, start a range, or open a
           "[:class:]"/"[.symbol.]"/"[=equiv=]" construct either.
           POSIX bracket expressions don't define an escape mechanism
           of their own, but shish's own quote-removal reuses a real
           backslash as its internal "protect this literal char"
           marker regardless of whether the character ends up inside
           an open bracket expression or not (case "[.]" produces the
           same "\[.\]" text as case [\.]/[\]] do) -- so this parser
           has to honor it here too, or an escaped "]"/"[" inside a
           class is misread as really closing/opening one
           (case-quoted-bracket-not-literal, fixes/82). */
        if(*pattern == '\\' && plen > 1 && !(flags & PATH_FNM_NOESCAPE)) {
          escaped = 1;
          pattern++;
          plen--;
        }

        /* if there is a closing bracket and it's not
           the first char the class is terminated */
        if(!escaped && *pattern == ']' && pattern != start)
          break;

        if(!escaped && *pattern == '[' && plen > 1 &&
           (pattern[1] == ':' || pattern[1] == '.' || pattern[1] == '=')) {
          /* MEMBER - "[:class:]" / "[.symbol.]" / "[=equiv=]" */
          char delim = pattern[1];
          unsigned int sublen = path_fnmatch_subexpr_len(pattern, plen, delim);

          if(sublen == 0) {
            /* unterminated -- treat the "[" itself as an ordinary
               literal member character, same as any other char
               falling through to the plain "else" branch below */
            res = (*pattern == *string);
            pattern++;
            plen--;
          } else if(delim == ':') {
            res = path_fnmatch_class(&pattern[2], sublen - 4, *string);
            pattern += sublen;
            plen -= sublen;
          } else {
            /* collating symbol / equivalence class: full locale-aware
               collation isn't implemented, so in the C/POSIX locale
               both degenerate to matching just the single enclosed
               character -- the only behavior a conforming
               application can rely on anyway. A multi-character
               symbol (no locale data to resolve it against) matches
               nothing rather than mismatching the bracket expression
               outright. */
            unsigned int symlen = sublen - 4;

            if(symlen == 1)
              res = (pattern[2] == *string);

            pattern += sublen;
            plen -= sublen;
          }
        } else {
          /* MEMBER - character range */
          if(!escaped && plen > 1 && pattern[1] == '-' && pattern[2] != ']') {
            /* check wheter char is within the specified range */
            res = (*string >= *pattern && *string <= pattern[2]);
            pattern += 3;
            plen -= 3;
          }
          /* MEMBER - literal character match */
          else {
            res = (*pattern == *string);
            pattern++;
            plen--;
          }
        }
        /* character class seems terminated and matched */
        if((res && !neg) || ((!res && neg) && *pattern == ']')) {
          while(plen && *pattern != ']') {
            /* skip an escaped char whole (backslash + the char it
               protects) so an escaped "]" further along the class
               doesn't end this scan early -- same escape rule as
               above, needed once matching has already succeeded and
               the rest of the class just needs skipping past. */
            if(*pattern == '\\' && plen > 1 && !(flags & PATH_FNM_NOESCAPE)) {
              pattern++;
              plen--;
            }

            pattern++;
            plen--;
          }
          pattern += !!plen;
          plen -= !!plen;
          string++;
          slen--;
          goto start;
        }
        /* not terminated but unmatched */
        else if(res && neg)
          break;
      }
    } break;
    case '\\': {
      /* do we escape chars? */
      if(!(flags & PATH_FNM_NOESCAPE)) {
        /* escape next character... */
        pattern++;
        plen--;
        /* ...if there is one */
        if(plen)
          goto match;
      }
      /* don't escape -> literal match */
      else
        goto match;
    } break;
    case '*': {
      /* this is the only situation where we really need to recurse */
      if((*string == '/' && (flags & PATH_FNM_PATHNAME)) ||
         path_fnmatch(pattern, plen, string + 1, slen - 1, flags)) {
        pattern++;
        plen--;
        goto start;
      }
      return 0;
    }
    case '?': {
      /* it can't match a / when we're matching a pathname */
      if(*string == '/' && (flags & PATH_FNM_PATHNAME))
        break;
      pattern++;
      plen--;
      string++;
      slen--;
    }
      goto start;
    default:
    match: {
      /* perform literal match */
      if(*pattern == *string) {
        pattern++;
        plen--;
        string++;
        slen--;
        goto start;
      }
    } break;
  }
  return PATH_FNM_NOMATCH;
}
