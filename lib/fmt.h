/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef FMT_H
#define FMT_H

#include "typedefs.h"
#include "uint32.h"
#include "uint64.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FMT_LONG 41        /* enough space to hold -2^127 in decimal, plus \0 */
#define FMT_ULONG 40       /* enough space to hold 2^128 - 1 in decimal, plus \0 */
#define FMT_8LONG 44       /* enough space to hold 2^128 - 1 in octal, plus \0 */
#define FMT_XLONG 33       /* enough space to hold 2^128 - 1 in hexadecimal, plus \0 */
#define FMT_LEN ((char*)0) /* convenient abbreviation */

#define FMT_ISO8601 63
/* The formatting routines do not append \0!
 * Use them like this: buf[fmt_ulong(buf, number)] = 0; */

/* convert signed src integer -23 to ASCII '-','2','3', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_long(char* dest, signed long src);

/* convert unsigned src integer 23 to ASCII '2','3', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_ulong(char* dest, unsigned long src);

/* convert unsigned src integer 0x23 to ASCII '2','3', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_xlong(char* dest, unsigned long src);

/* convert unsigned src integer 023 to ASCII '2','3', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_8long(char* dest, unsigned long src);

#define fmt_uint(dest, src) fmt_ulong(dest, src)
#define fmt_int(dest, src) fmt_long(dest, src)
#define fmt_xint(dest, src) fmt_xlong(dest, src)
#define fmt_8int(dest, src) fmt_8long(dest, src)

/* Like fmt_ulong, but prepend '0' while length is smaller than padto.
 * Does not truncate! */
size_t fmt_ulong0(char*, unsigned long src, size_t padto);

#define fmt_uint0(buf, src, padto) fmt_ulong0(buf, src, padto)

/* convert src double 1.7 to ASCII '1','.','7', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_double(char* dest, double d, int max, int prec);

/* if src is negative, write '-' and return 1.
 * if src is positive, write '+' and return 1.
 * otherwise return 0 */
size_t fmt_plusminus(char* dest, int src);

/* if src is negative, write '-' and return 1.
 * otherwise return 0. */
size_t fmt_minus(char* dest, int src);

/* copy str to dest until \0 byte, return number of copied bytes. */
size_t fmt_str(char* dest, const char* src);

/* copy str to dest until \0 byte or limit bytes copied.
 * return number of copied bytes. */
size_t fmt_strn(char* dest, const char* src, size_t limit);

/* "foo" -> "  foo"
 * write padlen - srclen spaces, if that is >= 0.  Then copy srclen
 * characters from src.  Truncate only if total length is larger than
 * maxlen.  Return number of characters written. */
size_t fmt_pad(char* dest, const char* src, size_t srclen, size_t padlen, size_t maxlen);

/* "foo" -> "foo  "
 * append padlen - srclen spaces after dest, if that is >= 0.  Truncate
 * only if total length is larger than maxlen.  Return number of
 * characters written. */
size_t fmt_fill(char* dest, size_t srclen, size_t padlen, size_t maxlen);

/* 1 -> "1", 4900 -> "4.9k", 2300000 -> "2.3M" */
size_t fmt_human(char* dest, uint64 l);

/* 1 -> "1", 4900 -> "4.8k", 2300000 -> "2.2M" */
size_t fmt_humank(char* dest, uint64 l);

/* "Sun, 06 Nov 1994 08:49:37 GMT" */
size_t fmt_httpdate(char* dest, time_t t);

#define FMT_UTF8 5
#define FMT_ASN1LENGTH 17 /* enough space to hold 2^128 - 1 */
#define FMT_ASN1TAG 19    /* enough space to hold 2^128 - 1 */
/* some variable length encodings for integers */
size_t fmt_utf8(char* dest, uint32 n);          /* can store 0 - 0x7fffffff */
size_t fmt_asn1derlength(char* dest, uint64 l); /* 0 - 0x7f: 1 byte, above that 1 + bytes_needed bytes */
size_t fmt_asn1dertag(char* dest, uint64 l);    /* 1 byte for each 7 bits; upper bit = more bytes coming */

/* internal functions, may be independently useful */
char fmt_tohex(char c);

#ifdef __BORLANDC__
#define fmt_strm(b, args) fmt_strm_internal(b, args, (char*)0)
#else
#define fmt_strm(b, ...) fmt_strm_internal(b, __VA_ARGS__, (char*)0)
#endif

size_t fmt_strm_internal(char* dest, ...);

#ifndef MAX_ALLOCA
#define MAX_ALLOCA 100000
#endif

#ifdef __BORLANDC__
#define fmt_strm_alloca(a, args)                                                                                       \
  ({                                                                                                                   \
    size_t len = fmt_strm((char*)0, a, args) + 1;                                                                      \
    char* c = (len < MAX_ALLOCA ? alloca(len) : 0);                                                                    \
    if(c)                                                                                                              \
      c[fmt_strm(c, a, args)] = 0;                                                                                     \
    c;                                                                                                                 \
  })
#else
#define fmt_strm_alloca(a, ...)                                                                                        \
  ({                                                                                                                   \
    size_t len = fmt_strm((char*)0, a, __VA_ARGS__) + 1;                                                               \
    char* c = (len < MAX_ALLOCA ? alloca(len) : 0);                                                                    \
    if(c)                                                                                                              \
      c[fmt_strm(c, a, __VA_ARGS__)] = 0;                                                                              \
    c;                                                                                                                 \
  })
#endif

size_t fmt_ulonglong(char* dest, uint64 i);
size_t fmt_longlong(char* dest, int64 i);
size_t fmt_xlonglong(char* dest, uint64 x);
size_t fmt_octal(char* dest, uint64 o);

size_t fmt_escapecharquotedprintable(char* dest, unsigned int ch);
size_t fmt_escapecharquotedprintableutf8(char* dest, unsigned int ch);

unsigned int fmt_hexb(void* out, const void* d, unsigned int len);
size_t fmt_xmlescape(char* dest, unsigned int ch);

#ifdef UINT32_H
size_t fmt_escapecharc(char* dest, uint32 ch);

size_t fmt_escapecharshell(char* dest, uint32 ch);
size_t fmt_escapecharjson(char* dest, uint32 ch, char quote);
#endif

#ifdef TAI_H
size_t fmt_tai(char* dest, const struct tai* ta);
#endif
size_t fmt_iso8601(char* dest, time_t t);

char fmt_tohex(char c);
size_t fmt_repeat(char* dest, const char* src, int n);

size_t fmt_escapecharxml(char*, unsigned int ch);
#ifdef __cplusplus
}
#endif
#endif /* defined FMT_H */
