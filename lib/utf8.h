#ifndef _UTF8
#define _UTF8 1

#include <stdlib.h>
#include <sys/types.h>
#include <wchar.h>
// typedef int wchar_t;

int u8len(const char*, size_t count);
size_t u8stowcs(wchar_t*, const char* pu, size_t count);
int u8swcslen(const char*);
int u8towc(wchar_t*, const char* u, size_t count);
size_t wcstou8s(char*, const wchar_t* pw, size_t count);
int wcsu8slen(const wchar_t*);
int wctou8(char*, wchar_t w);
int wcu8len(const wchar_t);

/* strict, bounds-checked UTF-8 on code points (see lib/utf8/u8decode.c)
 *
 *   u8decode  the code point at s[0..n) into *cp; returns its length in bytes,
 *             0 when n is 0, -1 when the bytes are not valid UTF-8 (bad lead
 *             or continuation byte, overlong form, surrogate, above U+10FFFF),
 *             -2 when they are a valid start cut off by the end (more input
 *             may complete it: a character can straddle two reads)
 *   u8encode  writes cp to dst (4 bytes room, no terminator); 0 when cp is not
 *             a Unicode scalar value
 *   u8charlen bytes of the character at s, 1 for an invalid or cut-off byte
 *   u8count   number of characters in s[0..n), same rule for invalid bytes
 *   u8skip    bytes taken up by the first k characters of s[0..n)
 */
int u8decode(const char* s, size_t n, unsigned* cp);
int u8encode(char* dst, unsigned cp);
size_t u8charlen(const char* s, size_t n);
size_t u8count(const char* s, size_t n);
size_t u8skip(const char* s, size_t n, size_t k);

#endif
