#include "../windoze.h"

#if WINDOWS_NATIVE
#include "../byte.h"
#include "../glob.h"
#include "../str.h"
#include "../utf8.h"

#include <errno.h>
#include <stdlib.h>
#include <windows.h>

#define NUM_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

/*
 * Helper functions
 */

/* pattern and results are UTF-8; the Win32 calls below are the wide (W) ones */
#define WBUF (1 + _MAX_PATH)
#define U8BUF (4 * WBUF)

static size_t
glob_fromw(char* dst, const WCHAR* w) {
  size_t n = u8fromu16(dst, U8BUF - 1, (const unsigned short*)w, lstrlenW(w), NULL);

  dst[n] = '\0';
  return n;
}

static int
glob_hasmagic(const char* s) {
  return s[str_chr(s, '?')] || s[str_chr(s, '*')];
}

/*
 * API functions
 */

/* It gives you back the matched contents of your pattern, so for Win32, th
 * directories must be included
 */

static int
glob_cmp(const void* a, const void* b) {
  return str_diff(*(char* const*)a, *(char* const*)b);
}

int
glob(const char* pattern, int flags, int (*errfunc)(const char*, int), glob_t* pglob) {
  int result;
  char szRelative[U8BUF], szPattern3[U8BUF], szName[U8BUF];
  size_t dirlen;
  WIN32_FIND_DATAW find_data;
  HANDLE hFind;
  WCHAR wPattern[WBUF], wTmp[WBUF];
  char* buffer;
  const char *leafMost, *effectivePattern = pattern;
  const WCHAR* wp;
  const int bMagic = glob_hasmagic(pattern);
  int bNoMagic = 0, bMagic0;
  size_t maxMatches = ~(size_t)(0), wn, i, leaf = 0;

  if(flags & GLOB_NOMAGIC)
    bNoMagic = !bMagic;

  if(flags & GLOB_LIMIT)
    maxMatches = (size_t)pglob->gl_matchc;

  wn = u8tou16((unsigned short*)wPattern, WBUF - 1, pattern, str_len(pattern), NULL);
  wPattern[wn] = 0;

  if(flags & GLOB_TILDE) {
    if('~' == pattern[0] && ('\0' == pattern[1] || '/' == pattern[1] || '\\' == pattern[1])) {
      DWORD dw = ExpandEnvironmentStringsW(L"%HOMEDRIVE%%HOMEPATH%", wTmp, WBUF - 1);

      /* dw is the number of chars *needed* (with the NUL) when wTmp was too
       * small: use the pattern verbatim then */
      if(0 != dw && dw <= WBUF) {
        size_t rest = WBUF - dw;

        for(i = 0; i < rest && i + 1 < wn; ++i)
          wTmp[dw - 1 + i] = wPattern[1 + i];

        wTmp[dw - 1 + i] = 0;
        byte_copy(wPattern, (dw - 1 + i + 1) * sizeof(WCHAR), wTmp);
        glob_fromw(szPattern3, wPattern);
        effectivePattern = szPattern3;
      }
    }
  }

  /* the last separator of the wide pattern; the directory part, as UTF-8, prefixes every result */
  for(wp = wPattern; *wp; ++wp)
    if(*wp == L'/' || *wp == L'\\')
      leaf = (size_t)(wp - wPattern) + 1;

  if(leaf) {
    wTmp[0] = 0;
    byte_copy(wTmp, leaf * sizeof(WCHAR), wPattern);
    wTmp[leaf] = 0;
    dirlen = glob_fromw(szRelative, wTmp);
  } else {
    szRelative[0] = '\0';
    dirlen = 0;
  }

  leafMost = effectivePattern + (leaf ? dirlen : 0);

  /* does the leaf component itself contain a wildcard anywhere, not
   * just as its first character -- used below to decide whether
   * dotfiles/"."/".." should be filtered out of a wildcard match
   * (they should, unless the pattern explicitly starts with '.') */
  bMagic0 = glob_hasmagic(leafMost);

  hFind = FindFirstFileW(wPattern, &find_data);
  buffer = NULL;

  pglob->gl_pathc = 0;
  pglob->gl_pathv = NULL;

  if(0 == (flags & GLOB_DOOFFS))
    pglob->gl_offs = 0;

  if(hFind == INVALID_HANDLE_VALUE) {
    if(NULL != errfunc)
      (void)errfunc(effectivePattern, (int)GetLastError());

    result = GLOB_NOMATCH;
  } else {
    int cbCurr = 0;
    size_t cbAlloc = 0, cMatches = 0;

    result = 0;

    do {
      size_t cch, namelen, new_cbAlloc;

      if(bMagic0 && 0 == (flags & GLOB_PERIOD))
        if('.' == find_data.cFileName[0])
          continue;

      namelen = glob_fromw(szName, find_data.cFileName);

      if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(flags & GLOB_ONLYFILE)
          continue;

        if(bMagic0 && GLOB_NODOTSDIRS == (flags & GLOB_NODOTSDIRS))
          /* Pattern must begin with '.' to match either dots directory */
          if(0 == str_diff(".", szName) || 0 == str_diff("..", szName))
            continue;

        if(flags & GLOB_MARK)
          szName[namelen++] = '/';

      } else {
        if(flags & GLOB_ONLYDIR)
          /* Skip all further actions, and get the next entry */
          continue;
      }

      szName[namelen] = '\0';
      cch = namelen + dirlen;

      new_cbAlloc = (size_t)cbCurr + cch + 1;

      if(new_cbAlloc > cbAlloc) {
        char* new_buffer;

        new_cbAlloc *= 2;
        new_cbAlloc = (new_cbAlloc + 31) & ~(31);
        new_buffer = (char*)realloc(buffer, new_cbAlloc);

        if(new_buffer == NULL) {
          result = GLOB_NOSPACE;
          free(buffer);
          buffer = NULL;
          break;
        }

        buffer = new_buffer;
        cbAlloc = new_cbAlloc;
      }

      byte_copy(buffer + cbCurr, dirlen, szRelative);
      byte_copy(buffer + cbCurr + dirlen, namelen + 1, szName);
      cbCurr += (int)cch + 1;

      ++cMatches;
    } while(FindNextFileW(hFind, &find_data) && cMatches != maxMatches);

    (void)FindClose(hFind);

    if(result == 0) {
      /* Now expand the buffer, to fit in all the pointers. */
      size_t cbPointers = (1 + cMatches + pglob->gl_offs) * sizeof(char*);
      char* new_buffer = (char*)realloc(buffer, cbAlloc + cbPointers);

      if(new_buffer == NULL) {
        result = GLOB_NOSPACE;
        free(buffer);
      } else {
        char **pp, **begin, **end, *next_str;

        buffer = new_buffer;

        (void)byte_copyr(new_buffer + cbPointers, cbAlloc, new_buffer);

        /* Handle the offsets. */
        begin = (char**)new_buffer;
        end = begin + pglob->gl_offs;

        for(; begin != end; ++begin)
          *begin = NULL;

        /* Lay the pointers down in the order the entries were found
         * in, then sort them alphabetically -- POSIX glob() sorts by
         * default and only skips that when GLOB_NOSORT is passed. */
        pp = (char**)new_buffer + pglob->gl_offs;
        end = pp + cMatches;

        for(begin = pp, next_str = buffer + cbPointers; begin != end; ++begin) {
          *begin = next_str;

          /* Find the next s. */
          next_str += 1 + str_len(next_str);
        }

        *begin = NULL;

        if(0 == (flags & GLOB_NOSORT))
          qsort(pp, cMatches, sizeof(char*), glob_cmp);

        /* Return results to caller. */
        pglob->gl_pathc = cMatches;
        pglob->gl_matchc = cMatches;
        pglob->gl_flags = 0;

        if(bMagic)
          pglob->gl_flags |= GLOB_MAGCHAR;

        pglob->gl_pathv = (char**)new_buffer;
      }
    }

    if(0 == cMatches)
      result = GLOB_NOMATCH;
  }

  if(GLOB_NOMATCH == result) {
    /* GLOB_TILDE_CHECK: a "~user" that failed to expand to a real
     * home directory should stay a genuine no-match, not get
     * synthesized back as a literal path below. */
    if((flags & GLOB_TILDE_CHECK) && effectivePattern == szPattern3) {
      /* leave result as GLOB_NOMATCH */
    } else if(bNoMagic || (flags & GLOB_NOCHECK)) {
      size_t cbNeeded = ((2 + pglob->gl_offs) * sizeof(char*)) + (1 + str_len(effectivePattern));
      char** pp = (char**)realloc(buffer, cbNeeded);

      if(NULL == pp) {
        result = GLOB_NOSPACE;
        free(buffer);
      } else {
        /* Handle the offsets. */
        char** begin = pp;
        char** end = pp + pglob->gl_offs;

        for(; begin != end; ++begin)
          *begin = NULL;

        /* Synthesis the pattern result. */

        pp[pglob->gl_offs] = (char*)(pp + 2 + pglob->gl_offs);
        pp[pglob->gl_offs + 1] = NULL;

        str_copy(pp[pglob->gl_offs], effectivePattern);

        /* Return results to caller. */
        pglob->gl_pathc = 1;
        pglob->gl_matchc = 1;
        pglob->gl_flags = 0;

        if(bMagic)
          pglob->gl_flags |= GLOB_MAGCHAR;

        pglob->gl_pathv = pp;

        result = 0;
      }
    }
  } else if(0 == result) {
    if((size_t)pglob->gl_matchc == maxMatches)
      result = GLOB_NOSPACE;
  }

  return result;
}

void
globfree(glob_t* pglob) {
  if(pglob != NULL) {
    free(pglob->gl_pathv);
    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;
  }
}
#endif