#define DEBUG_NOCOLOR 1
#include "../debug.h"

#ifdef DEBUG_ALLOC
#include "../sh.h"
#include <errno.h>

void
debug_free(const char* file, unsigned int line, void* ptr) {
  struct chunk* ch;
  /* search the pointer */
  for(ch = debug_heap; ch; ch = ch->next) {
    if(&ch[1] == ptr)
      break;
  }

  /* error invalid pointer! */
  if(ch == NULL) {
    errno = EFAULT;
    debug_error(file, line, "free");
    exit(1);
  }

  /* remove and free the chunk */
  if(ch) {
    if(ch->next)
      ch->next->pos = ch->pos;
    else
      debug_pos = ch->pos;
    *ch->pos = ch->next;
    free(ch);
  }
}
#endif /* DEBUG_OUTPUT */
