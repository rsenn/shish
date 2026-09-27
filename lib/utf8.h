#ifndef _UTF8
#define _UTF8 1

#include <stddef.h>

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

/* UTF-16 (Windows wide strings) to UTF-8: only whole characters, at most max bytes,
 * no terminator, a lone surrogate becomes U+FFFD; returns the bytes written and
 * stores the UTF-16 units consumed in *used (may be NULL) */
size_t u8fromu16(char* dst, size_t max, const unsigned short* w, size_t n, size_t* used);

/* 1 if the first non-empty of LC_ALL, LC_CTYPE, LANG (each may be NULL) names a UTF-8
 * codeset ("en_US.UTF-8", "C.utf8"); the shell reads its own variables, see sh_utf8() */
int u8locale(const char* lc_all, const char* lc_ctype, const char* lang);

/* the same operations for text that is UTF-8 (utf8 != 0) or plain bytes (0), so a
 * filter decides once, at init, and every character operation follows it
 *
 *   text_charlen    bytes of the next character
 *   text_charcount  characters in s[0..n)
 *   text_charskip   bytes taken up by the first k characters
 */
size_t text_charlen(int utf8, const char* s, size_t n);
size_t text_charcount(int utf8, const char* s, size_t n);
size_t text_charskip(int utf8, const char* s, size_t n, size_t k);

#endif
