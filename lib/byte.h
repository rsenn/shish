
/* this header file comes from libowfat, http://www.fefe.de/libowfat/ */
#ifndef BYTE_H
#define BYTE_H

/* for size_t: */
#include "typedefs.h"

#if !defined(_MSC_VER) && !defined(__MSYS__) && !defined(__CYGWIN__) && !defined(__BORLANDC__)
#include <stdint.h>
#endif

#ifndef __pure__
#define __pure__
#endif

/* byte_chr returns the smallest integer i between 0 and len-1
 * inclusive such that one[i] equals needle, or len if not found. */
size_t byte_chr(const void* haystack, size_t len, char needle) __pure__;

/* byte_rchr returns the largest integer i between 0 and len-1 inclusive
 * such that one[i] equals needle, or len if not found. */

/* byte_copy copies in[0] to out[0], in[1] to out[1], ... and in[len-1]
 * to out[len-1]. */
void byte_copy(void* out, size_t len, const void* in);

/* byte_copyr copies in[len-1] to out[len-1], in[len-2] to out[len-2],
 * ... and in[0] to out[0] */
void byte_copyr(void* out, size_t len, const void* in);

/* byte_diff returns negative, 0, or positive, depending on whether the
 * string a[0], a[1], ..., a[len-1] is lexicographically smaller
 * than, equal to, or greater than the string b[0], b[1], ...,
 * b[len-1]. When the strings are different, byte_diff does not read
 * bytes past the first difference. */
int byte_diff(const void* a, size_t len, const void* b) __pure__;

/* byte_zero sets the bytes out[0], out[1], ..., out[len-1] to 0 */
void byte_zero(void* out, size_t len);

void byte_fill(void* out, size_t len, int c);

size_t byte_equal(const void* s, size_t n, const void* t);

#if defined(__i386__) || defined(__x86_64__)
#define UNALIGNED_ACCESS_OK
#endif

/* read only trim-left */

size_t byte_trimr(char* x, size_t n, const char* trimchars, unsigned int trimcharslen);

void byte_replace(char* x, size_t n, char before, char after);

size_t byte_ccopy(void*, size_t count, const void* src, char c);

size_t byte_count(const void*, size_t, char c);

#endif
