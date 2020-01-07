#include "debug.h"

#ifdef DEBUG
#include "sh.h"
#include <assert.h>
#include <errno.h>

void*
debug_realloc(const char* file, unsigned int line, void* ptr, unsigned long n) {
  struct chunk *ch, *newch;

  /* handle null-pointer */
  if(ptr == NULL)
    return debug_alloc(file, line, n);

  /* search the pointer */
  for(ch = debug_heap; ch; ch = ch->next) {
    if(&ch[1] == ptr)
      break;
  }

  /* error invalid pointer! */
  if(ch == NULL) {
    errno = EFAULT;
    debug_error(file, line, "realloc");
    exit(1);
  }

  /* remove chunk first */
  if(ch->next)
    ch->next->pos = ch->pos;
  else
    debug_pos = ch->pos;

  *ch->pos = ch->next;

  assert(debug_pos != &ch->next);

  /* then realloc the chunk */
  if((newch = realloc(ch, n + sizeof(struct chunk)))) {
    /* relink it to the end */
    newch->next = *debug_pos;
    newch->pos = debug_pos;

    *debug_pos = newch;
    debug_pos = &newch->next;

    /* and reinitialize */
    newch->file = file;
    newch->line = line;
    newch->size = n;
    newch++;
  }

  return newch;
}
#endif /* DEBUG */
