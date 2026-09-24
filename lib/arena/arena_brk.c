#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1

#include "../arena.h"

#ifndef _WIN32
#include <stdint.h>
#include <unistd.h>

/* 16-aligned chunk at the current break; memory is never given back */
static void*
brk_get(size_t* size) {
  char* cur = sbrk(0);
  size_t pad;

  if(cur == (char*)-1)
    return NULL;

  pad = -(uintptr_t)cur & 15;
  *size = (*size + 15) & ~(size_t)15;

  if(*size > (size_t)INTPTR_MAX - pad || sbrk(pad + *size) == (void*)-1)
    return NULL;

  return cur + pad;
}

const struct arena_src arena_brk = {brk_get, NULL};
#endif
