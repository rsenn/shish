/**
 * @defgroup   strview
 * @brief      STRVIEW module.
 * @{
 */
#ifndef STRVIEW_H
#define STRVIEW_H

#include <stddef.h>

/* non-owning view of len bytes at str: the holder must not realloc or free
 * str. Same first two members as stralloc, so a stralloc's s/len can be
 * read through one (checked where a union relies on it, e.g. src/tree.h). */
typedef struct strview_s {
  const char* str;
  size_t len;
} strview;

#endif
/** @} */
