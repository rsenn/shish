/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef SCAN_H
#define SCAN_H

/* for size_t: */
#include "typedefs.h"

#include "uint32.h"
#include "uint64.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __pure__
#define __pure__
#endif

/* interpret src as ASCII decimal number, write number to dest and
 * return the number of bytes that were parsed */
size_t scan_ulong(const char* src, unsigned long* dest);
size_t scan_ulongn(const char* src, size_t n, unsigned long int* dest);

/* interpret src as ASCII hexadecimal number, write number to dest and
 * return the number of bytes that were parsed */
size_t scan_xlong(const char* src, unsigned long* dest);

/* interpret src as ASCII octal number, write number to dest and
 * return the number of bytes that were parsed */
size_t scan_8long(const char* src, unsigned long* dest);

/* interpret src as signed ASCII decimal number, write number to dest
 * and return the number of bytes that were parsed */
size_t scan_long(const char* src, signed long* dest);

size_t scan_longlong(const char* src, int64* dest);
size_t scan_ulonglong(const char* src, uint64* dest);
size_t scan_xlonglong(const char* src, uint64* dest);
size_t scan_octal(const char* src, uint64* dest);

size_t scan_uint(const char* src, unsigned int* dest);
size_t scan_xint(const char* src, unsigned int* dest);
size_t scan_8int(const char* src, unsigned int* dest);
size_t scan_int(const char* src, signed int* dest);

size_t scan_ushort(const char* src, unsigned short* dest);
size_t scan_xshort(const char* src, unsigned short* dest);
size_t scan_8short(const char* src, unsigned short* dest);
size_t scan_short(const char* src, signed short* dest);

size_t scan_xchar(const char*, unsigned char*);

/* interpret src as double precision floating point number,
 * write number to dest and return the number of bytes that were parsed */
size_t scan_double(const char* in, double* dest);

/* if *src == '-', set *dest to -1 and return 1.
 * if *src == '+', set *dest to 1 and return 1.
 * otherwise set *dest to 1 return 0. */
size_t scan_plusminus(const char* src, signed int* dest);

/* return the highest integer n <= limit so that isspace(in[i]) for all 0 <= i <= n */
size_t scan_whitenskip(const char* in, size_t limit) __pure__;

/* return the highest integer n <= limit so that !isspace(in[i]) for all 0 <= i <= n */
size_t scan_nonwhitenskip(const char* in, size_t limit) __pure__;

size_t scan_lineskip(const char* s, size_t limit);
size_t scan_line(const char* s, size_t limit);

/* return the highest integer n <= limit so that in[i] is element of
 * charset (ASCIIZ string) for all 0 <= i <= n */
size_t scan_charsetnskip(const char* in, const char* charset, size_t limit) __pure__;

/* return the highest integer n <= limit so that in[i] is not element of
 * charset (ASCIIZ string) for all 0 <= i <= n */
size_t scan_noncharsetnskip(const char* in, const char* charset, size_t limit) __pure__;

/* try to parse ASCII GMT date; does not understand time zones. */
/* example dates:
 *   "Sun, 06 Nov 1994 08:49:37 GMT"
 *   "Sunday, 06-Nov - 94 08:49:37 GMT"
 *   "Sun Nov  6 08:49:37 1994"
 */
size_t scan_httpdate(const char* in, time_t* t) __pure__;

/* some variable length encodings for integers */
size_t scan_utf8(const char* in, size_t len, uint32* n) __pure__;
size_t scan_asn1derlength(const char* in, size_t len, uint64* n) __pure__;
size_t scan_asn1dertag(const char* in, size_t len, uint64* n) __pure__;

/* a few internal function that might be useful independently */
/* convert from hex ASCII, return 0 to 15 for success or -1 for failure */
int scan_fromhex(unsigned char c);

size_t scan_8long(const char* src, unsigned long* dest);
size_t scan_octal(const char* src, uint64* dest);
size_t scan_8longn(const char* src, size_t n, unsigned long* dest);
size_t scan_int(const char* src, int* dest);
size_t scan_long(const char* src, long* dest);
size_t scan_longlong(const char* src, int64* dest);
size_t scan_longn(const char* src, size_t n, long* dest);
size_t scan_xlongn(const char* src, size_t n, unsigned long* dest);
size_t scan_pb_tag(const char* in, size_t len, size_t* fieldno, unsigned char* type);
size_t scan_pb_type0_sint(const char* in, size_t len, int64* l);
size_t scan_varint(const char* in, size_t len, uint64* n);

size_t scan_xmlescape(const char* src, char* dest);

size_t scan_utf8_sem(const char* in, size_t len, uint32* num);

size_t scan_eolskip(const char* s, size_t limit);

#ifdef __cplusplus
}
#endif

#endif
