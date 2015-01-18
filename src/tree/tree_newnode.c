#include "shell.h"
#include "byte.h"
#include "tree.h"

/* allocate a tree node
 * ----------------------------------------------------------------------- */
#ifdef DEBUG
union node *tree_newnodedebug(const char *file, unsigned int line, enum nod_id nod)
#else
union node *tree_newnode(enum nod_id nod)
#endif
{
  union node *ret;

#ifdef DEBUG
  if((ret = shell_allocdebug(file, line, sizeof(union node))))
#else
  if((ret = shell_alloc(sizeof(union node))))
#endif
  {
    byte_zero(ret, sizeof(union node));
    ret->id = nod;
  }
  return ret;
}
