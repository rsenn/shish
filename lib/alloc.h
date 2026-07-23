/**
 * @defgroup   alloc
 * @brief      ALLOC module.
 * @{
 */
#ifndef ALLOC_H
#define ALLOC_H

#include <stdlib.h>

void* alloc(unsigned long size);
void* alloc_zero(unsigned long size);
void* alloc_re(void* ptr, unsigned long size);
void* str_dup(const char* s);
#define alloc_free(p) free((p))

#endif
/** @} */
