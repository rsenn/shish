/* this header file comes from libowfat, http://www.fefe.de/libowfat/ */
#ifndef BYTE_H
#define BYTE_H

/* for size_t: */
#include "typedefs.h"

#if !defined(_MSC_VER) && !defined(__MSYS__) && !defined(__CYGWIN__) && !defined(__BORLANDC__)
#include <inttypes.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __pure__
#define __pure__
#endif

/* byte_chr returns the smallest integer i between 0 and len-1
 * inclusive such that one[i] equals needle, or len if not found. */
size_t byte_chr(const void* haystack, size_t len, char needle) __pure__;
size_t byte_chrs(const char*, size_t, char needle[], size_t nl);

/* byte_rchr returns the largest integer i between 0 and len-1 inclusive
 * such that one[i] equals needle, or len if not found. */
size_t byte_rchr(const void* haystack, size_t len, char needle) __pure__;

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

//#define byte_equal(s,n,t) (!byte_diff((s),(n),(t)))

int byte_equal_notimingattack(const void* a, size_t len, const void* b) __pure__;

void byte_fill(void* out, size_t len, int c);

int byte_case_diff(const void* x1, size_t len, const void* x2);
size_t byte_case_equal(const void* s, size_t len, const void* t);
size_t byte_count(const void* s, size_t n, char c);
size_t byte_equal(const void* s, size_t n, const void* t);
void byte_lower(void* s, size_t len);
void byte_upper(void* s, size_t len);

size_t byte_findb(const void* haystack, size_t hlen, const void* what, size_t wlen);
size_t byte_finds(const void* haystack, size_t hlen, const char* what);

#if defined(__i386__) || defined(__x86_64__)
#define UNALIGNED_ACCESS_OK
#endif

#ifdef STRALLOC_H
size_t byte_fmt(const char* in, size_t in_len, stralloc* out, size_t (*fmt_function)(char*, unsigned int ch));
size_t byte_scan(const char* in, size_t in_len, stralloc* out, size_t (*scan_function)(const char*, char*));
#endif

void byte_replace(char* x, size_t n, char before, char after);
size_t byte_ccopy(void* dst, size_t count, const void* src, char c);

/* read only trim-left */
const char* byte_triml(const char* x, size_t* len, const char* charset, unsigned int charsetlen);

size_t byte_trimr(char* x, size_t n, const char* trimchars, unsigned int trimcharslen);

static inline char*
byte_trim(char* x, size_t* n, const char* trimchars, unsigned int trimcharslen) {
  x = (char*)byte_triml(x, n, trimchars, trimcharslen);
  *n = byte_trimr(x, *n, trimchars, trimcharslen);
  return x;
}

size_t byte_camelize(char* x, size_t len);

#define byte_foreach(x, n, p) byte_foreach_skip(x, n, p, 1)
#define byte_foreach_skip(x, n, p, skip)                                                                               \
  for((p) = (void*)x; (void*)(p) != ((char*)(x) + (n)); (p) = (void*)(((char*)(p)) + (skip)))

#ifdef __cplusplus
}
#endif

#endif
