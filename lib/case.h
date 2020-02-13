/* this header file comes from libowfat, http://www.fefe.de/libowfat/ */
#ifndef CASE_H
#define CASE_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* turn upper case letters to lower case letters, ASCIIZ */
void case_lowers(char* s);
/* turn upper case letters to lower case letters, binary */
void case_lowerb(void* buf, size_t len);

int case_lowerc(int c);

/* like str_diff, ignoring case */
int case_diffs(const char*, const char*);
/* like byte_diff, ignoring case */
int case_diffb(const void*, size_t, const void*);

/* like str_start, ignoring case */
int case_starts(const char*, const char*);

#define case_equals(s, t) (!case_diffs((s), (t)))
#define case_equalb(s, n, t) (!case_diffb((s), (n), (t)))

size_t case_findb(const void*, size_t, const void*, size_t);
size_t case_finds(const void*, size_t, const char*);
size_t case_find(const void*, const void*);

#ifdef __cplusplus
}
#endif

#endif
