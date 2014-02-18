#include <shell.h>
#include <byte.h>
#include <str.h>

#ifdef DEBUG
extern void *shell_allocdebug(const char *file, unsigned int line, unsigned long n);

void *shell_strdupdebug(const char *file, unsigned int line, const char *s)
#else
void *shell_strdup(const char *s)
#endif /* DEBUG */
{
  unsigned long n;
  void *ptr;

  n = str_len(s);
  
#ifdef DEBUG
  ptr = shell_allocdebug(file, line, n + 1);
#else
  ptr = shell_alloc(n + 1);
#endif /* DEBUG */
  
  if(ptr)
    byte_copy(ptr, n + 1, s);

  return ptr;
}
