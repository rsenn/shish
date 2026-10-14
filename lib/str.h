/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef STR_H
#define STR_H

#include "typedefs.h"

#ifndef __pure__
#define __pure__
#endif

/* str_chr returns the index of the first occurance of needle or \0
   in haystack */
size_t str_chr(const char* haystack, char needle) __pure__;

/* str_chrn returns the index of the first character that is not c */
size_t str_chrn(const char* in, char c);

/* str_chrs returns the index of the first occurance of needles[nn] or \0
   in haystack */
size_t str_chrs(const char* in, const char needles[], size_t nn);

/* str_nchrs returns the index of the first occurance of a character
   not in needles[nn] or \0 in haystack */
size_t str_chrns(const char* in, const char needles[], size_t nn);

/* str_rchr returns the index of the last occurance of needle or \0
   in haystack  */
size_t str_rchr(const char* haystack, char needle) __pure__;

/* str_rchrs returns the index of the last occurance of needles[nn] or \0
   in haystack */
size_t str_rchrs(const char* haystack, char needles[], size_t nn) __pure__;

/* str_ndup makes a fresh alloc()'d copy of s[0..n), NUL-terminated
 * NULL if out of memory. */
char* str_ndup(const char* s, size_t n);

/* all the str_* functions that would also be implementable by
   string.h functions we only compile & link in when linking
   dynamically (to use the highly optimized/vectorized libc). */
#if LINK_STATIC

/* str_copy copies leading bytes from in to out until \0.
 * returns number of copied bytes. */
size_t str_copy(char* out, const char* in);
size_t str_copyn(char* out, const char* in, size_t max);

/* str_diff returns negative, 0, or positive, depending on whether the
 * string a[0], a[1], ..., a[n] == '\0' is lexicographically smaller than,
 * equal to, or greater than the string b[0], b[1], ..., b[m - 1] == '\0'.
 * If the strings are different, str_diff does not read bytes past the
 * first difference. */
int str_diff(const char* a, const char* b) __pure__;

/* str_diffn returns negative, 0, or positive, depending on whether the
 * string a[0], a[1], ..., a[n] == '\0' is lexicographically smaller than,
 * equal to, or greater than the string b[0], b[1], ..., b[m - 1] == '\0'.
 * If the strings are different, str_diffn does not read bytes past the
 * first difference. The strings will be considered equal if the first
 * limit characters match. */
int str_diffn(const char* a, const char* b, size_t limit) __pure__;

/* str_len returns the length of the string in bytes */
size_t str_len(const char* in);

/* in case we're linking dynamically, implement functions inline
   by means of libc string.h functions */
#else
#include <string.h>

//static inline size_t str_copyn(char* out, const char* in, size_t n);
size_t str_copy(char* out, const char* in);


#ifdef HAVE_MEMCCPY
static inline size_t
str_copyn(char* out, const char* in, size_t n) {
  char* t;

  if(!(t = memccpy(out, in, '\0', n))) {
    t = out + n;
    *t = '\0';
  } else {
    t--;
  }

  return (size_t)(t - out);
}
#else
size_t str_copyn(char* out, const char* in, size_t n);
#endif

#define str_diff(a, b) strcmp((a), (b))
#define str_diffn(a, b, n) strncmp((a), (b), (n))
#define str_len(s) strlen((s))
#endif

/* dietlibc doesn't have str(n)casecmp() */
#if LINK_STATIC || defined(__dietlibc__)

/* str_case_diff is like str_diff, but it lowercases uppercase characters */
int str_case_diff(const char* a, const char* b);

/* str_case_diff is like str_diffb, but it lowercases uppercase characters */
int str_case_diffn(const char* a, const char* b, size_t n);

#else
#define str_case_diff(a, b) strcasecmp((a), (b))
#define str_case_diffn(a, b, n) strncasecmp((a), (b), (n))
#endif

/* convenience shortcut to test for string equality */
#define str_equal(s, t) (!str_diff((s), (t)))

/* convenience shortcut to test if a character is contained
   within a string */
#define str_contains(s, ch) ((s)[str_chr((s), (ch))] != '\0')

#define str_foreach(s, ptr) for((ptr) = (void*)(s); *(const char*)(ptr); (ptr) = ((const char*)(ptr)) + 1)
#define str_foreach_skip(s, ptr, skipcall) \
  for((ptr) = (void*)(s); *(const char*)(ptr); (ptr) = ((const char*)(ptr)) + (skipcall))

#endif
