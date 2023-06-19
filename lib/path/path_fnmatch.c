#include "../path_internal.h"

/*#include "../buffer.h"*/
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support strings that are not nul-terminated
 */
#define NOTFIRST 0x80

int
path_fnmatch(const char* pattern,
             unsigned int plen,
             const char* string,
             unsigned int slen,
             int flags) {
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
        /* if there is a closing bracket and it's not
           the first char the class is terminated */
        if(*pattern == ']' && pattern != start)
          break;
        if(*pattern == '[' && pattern[1] == ':') {
          /* MEMBER - stupid POSIX char classes */
          /* TODO: implement them, but maybe not because POSIX sucks here! HARR
           * HARR */
        } else {
          /* MEMBER - character range */
          if(plen > 1 && pattern[1] == '-' && pattern[2] != ']') {
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
    match : {
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
