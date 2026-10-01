/* tcc has no __GNUC__; libarchive's archive_blake2.h wants it for
 * __attribute__((packed)).
 *
 *   limits.h first   -> INT_MAX etc. defined while __GNUC__ is still unset
 *   _GCC_LIMITS_H_   -> later limits.h includes skip #include_next (tcc has none)
 */
#include <limits.h>
#define _GCC_LIMITS_H_
#define __GNUC__ 2
