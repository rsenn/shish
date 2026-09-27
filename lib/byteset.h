#ifndef BYTESET_H
#define BYTESET_H

#include <stddef.h>

/**
 * @defgroup   byteset
 * @brief      a set of byte values as a 256-bit map
 *
 * The set is a plain `unsigned char set[BYTESET_SIZE]`, bit (c & 7) of byte
 * (c >> 3), so it can live in a struct, be copied with byte_copy() and be
 * passed around without a wrapper type. Bytes only: no locale collation, no
 * multibyte characters.
 * @{
 */

#define BYTESET_SIZE 32

void byteset_zero(unsigned char* set);
void byteset_add(unsigned char* set, unsigned c);
int byteset_has(const unsigned char* set, unsigned c);

/* adds lo..hi inclusive; -1 (nothing added) when lo > hi */
int byteset_range(unsigned char* set, unsigned lo, unsigned hi);

/* adds a POSIX class by name ("alpha", "digit", ..., as in [:alpha:]) in the
 * current locale; 1 if the name is known, else 0 */
int byteset_class(unsigned char* set, const char* name, size_t len);

/* every byte not in the set becomes a member and vice versa */
void byteset_invert(unsigned char* set);

/* adds the other case of every letter already in the set */
void byteset_icase(unsigned char* set);

/* the smallest member >= from, or -1; iterate with
 * for(c = byteset_next(s, 0); c >= 0; c = byteset_next(s, c + 1)) */
int byteset_next(const unsigned char* set, int from);

/** @} */
#endif
