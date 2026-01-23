/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef STR_H
#define STR_H

#include "typedefs.h"

#ifndef __pure__
#define __pure__
#endif

/* str_copy copies leading bytes from in to out until \0.
 * return number of copied bytes. */
size_t str_copy(char* out, const char* in);
size_t str_copyn(void* p1, const void* p2, size_t max);

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

int str_case_diff(const char* a, const char* b);
int str_case_diffn(const char* a, const char* b, size_t n);

size_t str_len(const char* in);

/* str_chr returns the index of the first occurance of needle or \0 in haystack
 */
size_t str_chr(const char* haystack, char needle) __pure__;

/* str_rchr returns the index of the last occurance of needle or \0 in haystack
 */
size_t str_rchr(const char* haystack, char needle) __pure__;

size_t str_rchrs(const char*, char needles[], size_t nn);

/* convenience shortcut to test for string equality */
#define str_equal(s, t) (!str_diff((s), (t)))

#define str_foreach(s, ptr) \
  for((ptr) = (void*)(s); *(const char*)(ptr); (ptr) = ((const char*)(ptr)) + 1)
#define str_foreach_skip(s, ptr, skipcall) \
  for((ptr) = (void*)(s); *(const char*)(ptr); (ptr) = ((const char*)(ptr)) + (skipcall))

#define str_containsc(s, ch) (((s)[str_chr((s), (ch))]) != 0)

#if !LINK_STATIC
#include <string.h>

#define str_len(s) strlen((s))
#define str_diff(a, b) strcmp((a), (b))
#define str_diffn(a, b, n) strncmp((a), (b), (n))
#ifndef __dietlibc__
#define str_case_diff(a, b) strcasecmp((a), (b))
#define str_case_diffn(a, b, n) strncasecmp((a), (b), (n))
#endif
#endif

#endif
