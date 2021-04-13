/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef FMT_H
#define FMT_H

#include "typedefs.h"
#include "uint32.h"
#include "uint64.h"

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

/* convert unsigned src integer 023 to ASCII '2','3', return length.
 * If dest is not NULL, write result to dest */
size_t fmt_8long(char* dest, unsigned long src);

/* Like fmt_ulong, but prepend '0' while length is smaller than padto.
 * Does not truncate! */
size_t fmt_ulong0(char*, unsigned long src, size_t padto);

size_t fmt_minus(char* dest, int src);

#ifdef UINT64_H
size_t fmt_8longlong(char* dest, uint64 i);
size_t fmt_ulonglong(char* dest, uint64 i);
size_t fmt_longlong(char* dest, int64 i);
size_t fmt_xlonglong(char* dest, uint64 x);
#endif

#ifdef UINT32_H
size_t fmt_escapecharc(char* dest, uint32 ch);
#endif

#endif /* defined FMT_H */
