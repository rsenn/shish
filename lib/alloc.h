/**
 * @defgroup   alloc
 * @brief      ALLOC module.
 * @{
 */
#ifndef ALLOC_H
#define ALLOC_H

#include <stdlib.h>

#ifndef DEBUG_ALLOC
void* alloc(unsigned long size);
void* alloc_zero(unsigned long size);
void* alloc_re(void* ptr, unsigned long size);
void* str_dup(const char* s);
#define alloc_free(p) free((p))
#else
extern void debug_free(const char* file, unsigned int line, void* p);
void* allocdebug(const char* file, unsigned int line, unsigned long size);
void* alloc_zerodebug(const char* file, unsigned int line, unsigned long size);
void* alloc_redebug(const char* file, unsigned int line, void* ptr, unsigned long size);
void* str_dupdebug(const char* file, unsigned int line, const char* s);
#define alloc(n) allocdebug(__FILE__, __LINE__, (n))
#define alloc_re(p, n) alloc_redebug(__FILE__, __LINE__, (p), (n))
#define str_dup(s) str_dupdebug(__FILE__, __LINE__, (s))
#define alloc_free(p) debug_free(__FILE__, __LINE__, (p))
#endif /* DEBUG_ALLOC */

#endif
/** @} */
