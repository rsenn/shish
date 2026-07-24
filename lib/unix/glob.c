#include "../windoze.h"

#if WINDOWS_NATIVE //&& !defined(_MSC_VER)

#include "../byte.h"
#include "../glob.h"
#include "../str.h"

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

static const char*
strrpbrk(const char* s, const char* charset) {
  char* part = NULL;
  const char* pch;

  for(pch = charset; *pch; ++pch) {
    char* p = strrchr(s, *pch);

    if(NULL != p) {
      if(NULL == part) {
        part = p;
      } else {
        if(part < p) {
          part = p;
        }
      }
    }
  }

  return part;
}

/*
 * API functions
 */

/* It gives you back the matched contents of your pattern, so for Win32, th
 * directories must be included
 */

static int
glob_cmp(const void* a, const void* b) {
  return lstrcmpA(*(char* const*)a, *(char* const*)b);
}

int
glob(const char* pattern, int flags, int (*errfunc)(const char*, int), glob_t* pglob) {
  int result;
  char szRelative[1 + _MAX_PATH];
  const char* file_part;
  size_t dirlen;
  WIN32_FIND_DATAA find_data;
  HANDLE hFind;
  char* buffer;
  char szPattern2[1 + _MAX_PATH];
  char szPattern3[1 + _MAX_PATH];
  const char* effectivePattern = pattern;
  const char* leafMost;
  const int bMagic = (NULL != strpbrk(pattern, "?*"));
  int bNoMagic = 0;
  int bMagic0;
  size_t maxMatches = ~(size_t)(0);

  if(flags & GLOB_NOMAGIC) {
    bNoMagic = !bMagic;
  }

  if(flags & GLOB_LIMIT) {
    maxMatches = (size_t)pglob->gl_matchc;
  }

  if(flags & GLOB_TILDE) {
    if('~' == pattern[0] && ('\0' == pattern[1] || '/' == pattern[1] || '\\' == pattern[1])) {
      DWORD dw;

      (void)lstrcpyA(&szPattern2[0], "%HOMEDRIVE%%HOMEPATH%");

      dw = ExpandEnvironmentStringsA(&szPattern2[0], &szPattern3[0], NUM_ELEMENTS(szPattern3) - 1);

      /* dw comes back as the number of chars *needed* (including the
       * NUL) when the destination was too small -- that can exceed
       * NUM_ELEMENTS(szPattern3), which would underflow the size
       * passed to lstrcpynA below into a huge value. Bail out and
       * fall back to using the pattern verbatim instead. */
      if(0 != dw && dw <= NUM_ELEMENTS(szPattern3)) {
        (void)lstrcpynA(&szPattern3[0] + dw - 1, &pattern[1], (int)(NUM_ELEMENTS(szPattern3) - dw));
        szPattern3[NUM_ELEMENTS(szPattern3) - 1] = '\0';

        effectivePattern = szPattern3;
      }
    }
  }

  file_part = strrpbrk(effectivePattern, "\\/");

  if(NULL != file_part) {
    leafMost = ++file_part;

    (void)lstrcpyA(szRelative, effectivePattern);
    szRelative[file_part - effectivePattern] = '\0';
    dirlen = (size_t)(file_part - effectivePattern);
  } else {
    szRelative[0] = '\0';
    leafMost = effectivePattern;
    dirlen = 0;
  }

  /* does the leaf component itself contain a wildcard anywhere, not
   * just as its first character -- used below to decide whether
   * dotfiles/"."/".." should be filtered out of a wildcard match
   * (they should, unless the pattern explicitly starts with '.') */
  bMagic0 = (NULL != strpbrk(leafMost, "?*"));

  hFind = FindFirstFileA(effectivePattern, &find_data);
  buffer = NULL;

  pglob->gl_pathc = 0;
  pglob->gl_pathv = NULL;

  if(0 == (flags & GLOB_DOOFFS)) {
    pglob->gl_offs = 0;
  }

  if(hFind == INVALID_HANDLE_VALUE) {
    if(NULL != errfunc) {
      (void)errfunc(effectivePattern, (int)GetLastError());
    }

    result = GLOB_NOMATCH;
  } else {
    int cbCurr = 0;
    size_t cbAlloc = 0;
    size_t cMatches = 0;

    result = 0;

    do {
      int cch;
      size_t new_cbAlloc;

      if(bMagic0 && 0 == (flags & GLOB_PERIOD)) {
        if('.' == find_data.cFileName[0]) {
          continue;
        }
      }

      if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(flags & GLOB_ONLYFILE) {
          continue;
        }

        if(bMagic0 && GLOB_NODOTSDIRS == (flags & GLOB_NODOTSDIRS)) {
          /* Pattern must begin with '.' to match either dots directory */
          if(0 == lstrcmpA(".", find_data.cFileName) || 0 == lstrcmpA("..", find_data.cFileName)) {
            continue;
          }
        }

        if(flags & GLOB_MARK) {
          (void)lstrcatA(find_data.cFileName, "/");
        }
      } else {
        if(flags & GLOB_ONLYDIR) {
          /* Skip all further actions, and get the next entry */
          continue;
        }
      }

      cch = lstrlenA(find_data.cFileName) + (int)dirlen;

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

      (void)lstrcpynA(buffer + cbCurr, szRelative, (int)(1 + dirlen));
      (void)lstrcatA(buffer + cbCurr, find_data.cFileName);
      cbCurr += cch + 1;

      ++cMatches;
    } while(FindNextFileA(hFind, &find_data) && cMatches != maxMatches);

    (void)FindClose(hFind);

    if(result == 0) {
      /* Now expand the buffer, to fit in all the pointers. */
      size_t cbPointers = (1 + cMatches + pglob->gl_offs) * sizeof(char*);
      char* new_buffer = (char*)realloc(buffer, cbAlloc + cbPointers);

      if(new_buffer == NULL) {
        result = GLOB_NOSPACE;
        free(buffer);
      } else {
        char** pp;
        char** begin;
        char** end;
        char* next_str;

        buffer = new_buffer;

        (void)byte_copyr(new_buffer + cbPointers, cbAlloc, new_buffer);

        /* Handle the offsets. */
        begin = (char**)new_buffer;
        end = begin + pglob->gl_offs;

        for(; begin != end; ++begin) {
          *begin = NULL;
        }

        /* Lay the pointers down in the order the entries were found
         * in, then sort them alphabetically -- POSIX glob() sorts by
         * default and only skips that when GLOB_NOSORT is passed. */
        pp = (char**)new_buffer + pglob->gl_offs;
        end = pp + cMatches;

        for(begin = pp, next_str = buffer + cbPointers; begin != end; ++begin) {
          *begin = next_str;

          /* Find the next s. */
          next_str += 1 + lstrlenA(next_str);
        }
        *begin = NULL;

        if(0 == (flags & GLOB_NOSORT)) {
          qsort(pp, cMatches, sizeof(char*), glob_cmp);
        }

        /* Return results to caller. */
        pglob->gl_pathc = cMatches;
        pglob->gl_matchc = cMatches;
        pglob->gl_flags = 0;

        if(bMagic) {
          pglob->gl_flags |= GLOB_MAGCHAR;
        }
        pglob->gl_pathv = (char**)new_buffer;
      }
    }

    if(0 == cMatches) {
      result = GLOB_NOMATCH;
    }
  }

  if(GLOB_NOMATCH == result) {
    /* GLOB_TILDE_CHECK: a "~user" that failed to expand to a real
     * home directory should stay a genuine no-match, not get
     * synthesized back as a literal path below. */
    if((flags & GLOB_TILDE_CHECK) && effectivePattern == szPattern3) {
      /* leave result as GLOB_NOMATCH */
    } else if(bNoMagic || (flags & GLOB_NOCHECK)) {
      size_t cbNeeded = ((2 + pglob->gl_offs) * sizeof(char*)) + (1 + strlen(effectivePattern));
      char** pp = (char**)realloc(buffer, cbNeeded);

      if(NULL == pp) {
        result = GLOB_NOSPACE;
        free(buffer);
      } else {
        /* Handle the offsets. */
        char** begin = pp;
        char** end = pp + pglob->gl_offs;

        for(; begin != end; ++begin) {
          *begin = NULL;
        }

        /* Synthesis the pattern result. */

        pp[pglob->gl_offs] = (char*)(pp + 2 + pglob->gl_offs);
        pp[pglob->gl_offs + 1] = NULL;

        str_copy(pp[pglob->gl_offs], effectivePattern);

        /* Return results to caller. */
        pglob->gl_pathc = 1;
        pglob->gl_matchc = 1;
        pglob->gl_flags = 0;

        if(bMagic) {
          pglob->gl_flags |= GLOB_MAGCHAR;
        }
        pglob->gl_pathv = pp;

        result = 0;
      }
    }
  } else if(0 == result) {
    if((size_t)pglob->gl_matchc == maxMatches) {
      result = GLOB_NOSPACE;
    }
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
