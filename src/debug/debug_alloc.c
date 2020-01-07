#include "debug.h"

#ifdef DEBUG
struct chunk* debug_heap = NULL;
struct chunk** debug_pos = &debug_heap;

void*
debug_alloc(const char* file, unsigned int line, unsigned long size) {
  /* try to allocate memory */
  struct chunk* ch = malloc(size + sizeof(struct chunk));

  /* if succeeded, initialize chunk and link it */
  if(ch) {
    /* link it to the end */
    ch->next = *debug_pos;
    ch->pos = debug_pos;

    *debug_pos = ch;
    debug_pos = &ch->next;

    /* initialize */
    ch->size = size;
    ch->file = file;
    ch->line = line;

    ch++;
  }

  return ch;
}
#endif /* DEBUG */
