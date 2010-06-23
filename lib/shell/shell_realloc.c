#include <shell.h>

#ifdef DEBUG
extern void *debug_realloc(const char *file, unsigned int line, void *ptr, unsigned long size);

void *shell_reallocdebug(const char *file, unsigned int line, void *ptr, unsigned long size)
#else
void *shell_realloc(void *ptr, unsigned long size)
#endif /* DEBUG */
{
  void *newptr;

  if(ptr == NULL)
#ifdef DEBUG
    return shell_allocdebug(file, line, size);
#else
    return shell_alloc(size);
#endif /* DEBUG */
  
#ifdef DEBUG
  newptr = debug_realloc(file, line, ptr, size);
#else
  newptr = realloc(ptr, size);
#endif /* DEBUG */
  
  /* exit if failed */
  if(newptr == NULL)
  {
    shell_error("realloc");
    exit(1);
  }  
  
  /* return pointer otherwise */
  return newptr;
}
