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

/* interpret src as ASCII octal number, write number to dest and
 * return the number of bytes that were parsed */
size_t scan_8long(const char* src, unsigned long* dest);

#ifdef UINT64_H
size_t scan_longlong(const char* src, int64* dest);
size_t scan_ulonglong(const char* src, uint64* dest);
size_t scan_xlonglong(const char* src, uint64* dest);
#endif

size_t scan_uint(const char* src, unsigned int* dest);
size_t scan_8int(const char* src, unsigned int* dest);
size_t scan_int(const char* src, signed int* dest);

size_t scan_8short(const char* src, unsigned short* dest);

/* return the highest integer n <= limit so that in[i] is element of
 * charset (ASCIIZ string) for all 0 <= i <= n */
size_t scan_charsetnskip(const char* in, const char* charset, size_t limit) __pure__;

/* return the highest integer n <= limit so that in[i] is not element of
 * charset (ASCIIZ string) for all 0 <= i <= n */
size_t scan_noncharsetnskip(const char* in, const char* charset, size_t limit) __pure__;

size_t scan_nonwhitenskip(const char*, size_t limit);
size_t scan_whitenskip(const char*, size_t limit);

/* convert from hex ASCII, return 0 to 15 for success or -1 for failure */
int scan_fromhex(unsigned char c);

size_t scan_8longn(const char* src, size_t n, unsigned long* dest);

#ifdef UINT64_H
size_t scan_8longlong(const char* src, uint64* dest);
size_t scan_8longlongn(const char* src, size_t n, uint64* dest);
#endif

#ifdef __cplusplus
}
#endif

#endif
